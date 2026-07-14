#include "../include/Assets/AssetManager.hpp"

#include "Core/Profile.hpp"

namespace axm {
    AssetHandle AssetLoadInfo::ToHandle() const { return { String(m_Path.string()), m_AssetType }; }

    AssetHandle
    AssetManager::LoadAsset(const Filesystem::path& path, const AssetType& assetType, const OnLoadedFn& onLoaded) {
        PROFILE_SCOPE()
        if (!Filesystem::exists(path.c_str())) {
            return { };
        }

        if (!p_AssetFactories.contains(assetType)) {
            AXM_LOG_ERROR("AssetManager : No factory provided for asset load with type {}", static_cast<u8>(assetType));
            return { };
        }

        // remove working directory from path (if it is part of the provided path)
        const String wd(Filesystem::current_path().string());
        String       tmp_path = String(path.string());
        if (tmp_path.find(wd) != std::string::npos) {
            tmp_path.erase(tmp_path.find(wd), wd.length());

            for (int i = 0; i < 2; i++) {
                if (tmp_path[0] != '\\' && tmp_path[0] != '/') {
                    break;
                }
                tmp_path.erase(0, 1);
            }
        }

        AssetHandle   handle(tmp_path, assetType);
        AssetLoadInfo loadInfo { .m_Path = tmp_path, .m_AssetType = assetType, .m_OnLoadedCallback = onLoaded };

        for (auto& queued_load: p_QueuedLoads) {
            if (loadInfo == queued_load) {
                return queued_load.ToHandle();
            }
        }

        p_QueuedLoads.push_back(loadInfo);

        return handle;
    }

    void AssetManager::UnloadAsset(const AssetHandle& handle) {
        PROFILE_SCOPE()
        // Asset is not loaded
        if (!p_LoadedAssets.contains(handle)) {
            return;
        }

        // make sure we have a provided function to unload the asset
        if (!p_AssetFactories.contains(handle.m_AssetType)) {
            // TODO: Get enum str value
            AXM_LOG_ERROR("No provided AssetFactory for AssetType {}", static_cast<u8>(handle.m_AssetType));
            return;
        }

        // Enqueue unload
        p_QueuedUnloads.push_back(handle);
    }

    Asset* AssetManager::GetAsset(const AssetHandle& handle) {
        PROFILE_SCOPE()
        if (p_LoadedAssets.find(handle) == p_LoadedAssets.end()) {
            return nullptr;
        }

        return p_LoadedAssets[handle].get();
    }

    AssetState AssetManager::GetAssetLoadProgress(const AssetHandle& handle) {
        PROFILE_SCOPE()
        for (auto& queued: p_QueuedLoads) {
            if (queued.ToHandle() == handle) {
                return AssetState::Loading;
            }
        }

        if (std::find(p_QueuedUnloads.begin(), p_QueuedUnloads.end(), handle) != p_QueuedUnloads.end()) {
            return AssetState::Unloading;
        }

        if (p_LoadedAssets.find(handle) != p_LoadedAssets.end()) {
            return AssetState::Loaded;
        }

        return AssetState::NotLoaded;
    }

    bool AssetManager::AnyAssetsLoading() const {
        PROFILE_SCOPE()
        return !p_QueuedUnloads.empty() || !p_QueuedLoads.empty() || !p_InFlightLoads.empty()
               || !p_InFlightTransients.empty();
    }

    bool AssetManager::AnyAssetsUnloading() const {
        PROFILE_SCOPE()
        return !p_QueuedUnloads.empty();
    }

    void AssetManager::WaitAllAssets() {
        PROFILE_SCOPE()
        while (AnyAssetsLoading()) {
            Update();
        }
    }

    void AssetManager::WaitAllUnloads() {
        PROFILE_SCOPE()
        while (AnyAssetsUnloading()) {
            Update();
        }
    }

    void AssetManager::UnloadAllAssets() {
        PROFILE_SCOPE()
        Vector<AssetHandle> assetsRemaining { };

        for (auto& [handle, asset]: p_LoadedAssets) {
            assetsRemaining.push_back(handle);
        }

        for (auto& handle: assetsRemaining) {
            UnloadAsset(handle);
        }

        WaitAllUnloads();
    }

    void AssetManager::Update() {
        PROFILE_SCOPE()
        if (!AnyAssetsLoading()) {
            return;
        }

        auto remainingMainThreadTasks = kMaxMainThreadTasksPerTick;
        auto remainingAsyncTasks      = kMaxAsyncTasksPerTick;

        HandlePendingLoads(remainingAsyncTasks);
        HandleUnloads(remainingMainThreadTasks);
        HandleAsyncTasks(remainingMainThreadTasks);
        HandleTransients(remainingMainThreadTasks);
    }

    void AssetManager::Shutdown() {
        PROFILE_SCOPE()
        WaitAllAssets();
        WaitAllUnloads();
        UnloadAllAssets();
    }

    void AssetManager::HandlePendingLoads(u16& remainingTasks) {
        PROFILE_SCOPE()

        auto bound = std::min(static_cast<size_t>(remainingTasks), p_QueuedLoads.size());

        for (auto i = 0; i < bound; i++) {
            // get first load info
            auto& loadInfo = p_QueuedLoads.front();
            // get the asset type factory
            auto const* factory = p_AssetFactories[loadInfo.m_AssetType].get();
            // dispatch initial async load task
            // p_PendingLoadTasks.emplace(handle, std::move(std::async(std::launch::async, fn, info.m_Path)));
            auto fn = [factory](const Filesystem::path& path) { return factory->LoadAsset(path); };

            p_InFlightLoads.push_back({ .m_Path         = loadInfo.m_Path,
                                        .m_AssetType    = loadInfo.m_AssetType,
                                        .m_Task         = std::async(std::launch::async, fn, loadInfo.m_Path),
                                        .m_LoadCallback = p_QueuedLoads.front().m_OnLoadedCallback });
            // erase queued load
            p_QueuedLoads.erase(p_QueuedLoads.begin());
            remainingTasks--;
        }
    }

    void AssetManager::HandleAsyncTasks(u16& remainingTasks) {
        PROFILE_SCOPE()
        auto bound = std::min(static_cast<size_t>(remainingTasks), p_InFlightLoads.size());

        for (auto i = 0; i < bound; i++) {
            if (IsFutureReady(p_InFlightLoads.front().m_Task)) {
                // get asset load result
                auto result = p_InFlightLoads.front().m_Task.get();
                // handle result & enqueue new loads
                for (auto& newLoad: result.m_NewAssetTasks) {
                    LoadAsset(newLoad.m_Path, newLoad.m_AssetType, newLoad.m_OnLoadedCallback);
                }
                // if asset is fully loaded, transition to loaded state
                if (std::holds_alternative<Asset*>(result.m_Next)) {
                    auto* asset = std::get<Asset*>(result.m_Next);
                    TransitionAssetToLoaded(asset, p_InFlightLoads.front().m_LoadCallback);
                    // decrement in case we had a callback to process
                    remainingTasks--;
                }
                // if transient, add to list for processing
                else if (std::holds_alternative<AssetTransient*>(result.m_Next)) {
                    auto* transient = std::get<AssetTransient*>(result.m_Next);

                    p_InFlightTransients.push_back({ .m_Transient    = Unique<AssetTransient>(transient),
                                                     .m_LoadCallback = p_InFlightLoads.front().m_LoadCallback });
                }
                // error
                else {
                    const auto error = std::get<AssetErrorMessage>(result.m_Next);
                    AXM_LOG_ERROR("AssetManager : Failed to load asset : {} : {}",
                                  p_InFlightLoads.front().m_Path.string().c_str(),
                                  error.m_Message);
                }

                p_InFlightLoads.erase(p_InFlightLoads.begin());
                remainingTasks--;
            }
        }
    }

    void AssetManager::HandleTransients(u16& remainingTasks) {
        PROFILE_SCOPE()
        auto bound = std::min(static_cast<size_t>(remainingTasks), p_InFlightTransients.size());
        for (auto i = 0; i < bound; i++) {
            // Process one step of the first of the in flight transients
            const auto type      = p_InFlightTransients.front().m_Transient->m_AssetType;
            auto*      transient = p_InFlightTransients.front().m_Transient.get();
            p_AssetFactories[type]->ProcessAssetTransient(transient);

            // if transient stage is finished, move to loaded state and delete transient
            if (transient->m_CurrentStep == transient->m_NumSteps) {
                TransitionAssetToLoaded(transient->m_AssetDataPtr, p_InFlightTransients.front().m_LoadCallback);
                p_InFlightTransients.erase(p_InFlightTransients.begin());
                // do an extra step in case we have a callback
                remainingTasks--;
            }
            remainingTasks--;
        }
    }

    void AssetManager::HandleUnloads(u16& remainingTasks) {
        PROFILE_SCOPE()
        auto bound = std::min(static_cast<size_t>(remainingTasks), p_QueuedUnloads.size());

        for (auto i = 0; i < bound; i++) {
            const auto  type   = p_QueuedUnloads.front().m_AssetType;
            const auto& handle = p_QueuedUnloads.front();
            p_AssetFactories[type]->UnloadAsset(p_LoadedAssets[handle].get());

            p_LoadedAssets.erase(handle);
            p_QueuedUnloads.erase(p_QueuedUnloads.begin());
            remainingTasks--;
        }
    }

    void AssetManager::TransitionAssetToLoaded(Asset* asset, OnLoadedFn loadCallback) {
        PROFILE_SCOPE()

        if (asset == nullptr) {
            AXM_LOG_ERROR("TransitionAssetToLoaded : Asset is nullptr!");
            return;
        }

        p_LoadedAssets.emplace(asset->m_Handle, Unique<Asset>(asset));

        if (loadCallback != nullptr) {
            loadCallback(asset);
        }
    }
} // namespace axm
