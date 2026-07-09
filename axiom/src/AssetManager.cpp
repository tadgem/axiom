#include "../include/Assets/AssetManager.hpp"

#include "Core/Profile.hpp"

namespace axm {
    AssetHandle AssetLoadInfo::ToHandle() const { return { path, type }; }

    bool
    AssetManager::ProvideAssetFactory(const AssetType& type, LoadAssetCallback onLoad, UnloadAssetCallback onUnload) {
        PROFILE_SCOPE();
        if (p_AssetFactories.find(type) != p_AssetFactories.end()) {
            return false;
        }

        const Pair funcs { onLoad, onUnload };

        p_AssetFactories[type] = funcs;
        return true;
    }

    AssetHandle
    AssetManager::LoadAsset(const String& path, const AssetType& assetType, OnAssetLoadedCallback onAssetLoaded) {
        PROFILE_SCOPE();
        if (!Filesystem::exists(path.c_str())) {
            return { };
        }

        const String wd(Filesystem::current_path().string());
        String       tmp_path = path;
        if (path.find(wd) != std::string::npos) {
            tmp_path.erase(tmp_path.find(wd), wd.length());

            for (int i = 0; i < 2; i++) {
                if (tmp_path[0] != '\\' && tmp_path[0] != '/') {
                    break;
                }
                tmp_path.erase(0, 1);
            }
        }

        AssetHandle   handle(tmp_path, assetType);
        AssetLoadInfo loadInfo { tmp_path, assetType };

        for (auto& queued_load: p_QueuedLoads) {
            if (loadInfo == queued_load) {
                return queued_load.ToHandle();
            }
        }

        p_QueuedLoads.push_back(loadInfo);

        if (onAssetLoaded != nullptr) {
            p_AssetLoadCallbacks.emplace(handle, onAssetLoaded);
        }

        return handle;
    }

    void AssetManager::UnloadAsset(const AssetHandle& handle) {
        PROFILE_SCOPE();
        // Asset is not loaded
        if (p_LoadedAssets.contains(handle)) {
            return;
        }

        // make sure we have a provided function to unload the asset
        if (!p_AssetFactories.contains(handle.m_AssetType)) {
            // TODO: Get enum str value
            AXM_LOG_ERROR("No provided AssetFactory for AssetType {}", static_cast<u8>(handle.m_AssetType));
            return;
        }

        // Enqueue unload
        p_PendingUnloadCallbacks.emplace(handle, p_AssetFactories[handle.m_AssetType].second);
    }

    Asset* AssetManager::GetAsset(const AssetHandle& handle) {
        PROFILE_SCOPE();
        if (p_LoadedAssets.find(handle) == p_LoadedAssets.end()) {
            return nullptr;
        }

        return p_LoadedAssets[handle].get();
    }

    AssetLoadProgress AssetManager::GetAssetLoadProgress(const AssetHandle& handle) {
        PROFILE_SCOPE();
        for (auto& queued: p_QueuedLoads) {
            if (queued.ToHandle() == handle) {
                return AssetLoadProgress::Loading;
            }
        }

        if (p_PendingLoadTasks.find(handle) != p_PendingLoadTasks.end()
            || p_PendingSyncCallbacks.find(handle) != p_PendingSyncCallbacks.end()) {
            return AssetLoadProgress::Loading;
        }

        if (p_PendingUnloadCallbacks.find(handle) != p_PendingUnloadCallbacks.end()) {
            return AssetLoadProgress::Unloading;
        }

        if (p_LoadedAssets.find(handle) != p_LoadedAssets.end()) {
            return AssetLoadProgress::Loaded;
        }

        return AssetLoadProgress::NotLoaded;
    }

    bool AssetManager::AnyAssetsLoading() const {
        PROFILE_SCOPE();
        return !p_PendingLoadTasks.empty() || !p_PendingSyncCallbacks.empty() || !p_PendingUnloadCallbacks.empty()
               || !p_QueuedLoads.empty();
    }

    bool AssetManager::AnyAssetsUnloading() const {
        PROFILE_SCOPE();
        return !p_PendingUnloadCallbacks.empty();
    }

    void AssetManager::WaitAllAssets() {
        PROFILE_SCOPE();
        while (AnyAssetsLoading()) {
            Update();
        }
    }

    void AssetManager::WaitAllUnloads() {
        PROFILE_SCOPE();
        while (AnyAssetsUnloading()) {
            Update();
        }
    }

    void AssetManager::UnloadAllAssets() {
        PROFILE_SCOPE();
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
        PROFILE_SCOPE();
        if (!AnyAssetsLoading()) {
            return;
        }

        HandleCallbacks();
        HandlePendingLoads();
        HandleAsyncTasks();
    }

    void AssetManager::Shutdown() {
        PROFILE_SCOPE();
        WaitAllAssets();
        WaitAllUnloads();
        UnloadAllAssets();
    }

    void AssetManager::HandleCallbacks() {
        PROFILE_SCOPE();
        u16                 processedCallbacks = 0;
        Vector<AssetHandle> clears;

        for (auto& [handle, asset]: p_PendingSyncCallbacks) {
            if (processedCallbacks == kCallbackTasksPerUpdate) {
                break;
            }

            bool transient = false;

            for (u16 i = 0; i < kCallbackTasksPerUpdate - processedCallbacks; i++) {
                // does asset have any intermediate steps?
                if (std::holds_alternative<AssetTransientData*>(asset.m_Next)) {
                    // has transient
                    asset.m_SyncAssetCallbacks.back()(std::get<AssetTransientData*>(asset.m_Next));
                    asset.m_SyncAssetCallbacks.pop_back();
                    processedCallbacks++;
                    transient = true;

                }
                // if not just move the asset to the loaded state.
                else if (std::holds_alternative<Asset*>(asset.m_Next)) {
                    Asset* a = std::get<Asset*>(asset.m_Next);
                    TransitionAssetToLoaded(handle, a);
                } else if (i >= asset.m_SyncAssetCallbacks.size())
                    break;
                // or we have pwoblems.
                else {
                    AXM_ASSERT_NOT_REACHED();
                }
            }

            if (asset.m_SyncAssetCallbacks.empty()) {
                clears.push_back(handle);

                if (transient) {
                    AssetTransientData* t = std::get<AssetTransientData*>(p_PendingSyncCallbacks[handle].m_Next);
                    Asset*              a = t->m_AssetDataPtr;
                    TransitionAssetToLoaded(handle, a);
                    AXM_DELETE(t);
                }
            }
        }
        for (auto& handle: clears) {
            p_PendingSyncCallbacks.erase(handle);
        }

        clears.clear();

        for (auto& [handle, callback]: p_PendingUnloadCallbacks) {
            if (processedCallbacks == kCallbackTasksPerUpdate)
                break;
            callback(p_LoadedAssets[handle].get());
            clears.push_back(handle);
            processedCallbacks++;
        }

        for (auto& handle: clears) {
            p_PendingUnloadCallbacks.erase(handle);
            p_LoadedAssets[handle].reset();
            p_LoadedAssets.erase(handle);
        }
    }

    void AssetManager::HandlePendingLoads() {
        PROFILE_SCOPE();
        while (p_PendingLoadTasks.size() <= kMaxAsyncTasksInFlight && !p_QueuedLoads.empty()) {
            auto& info = p_QueuedLoads.front();
            DispatchAssetLoadTask(info.ToHandle(), info);
            p_QueuedLoads.erase(p_QueuedLoads.begin());
        }
    }

    // TODO: This + HandleSyncTasks are a bit dodgy, might need to rewrite this.
    // lets hook up asset manager specific allocators ASAP,
    // have a feeling this is very memory inefficient.

    void AssetManager::HandleAsyncTasks() {
        PROFILE_SCOPE();
        Vector<AssetHandle> finished;
        for (auto& [handle, future]: p_PendingLoadTasks) {
            if (IsFutureReady(future)) {
                finished.push_back(handle);
            }
        }

        for (auto& handle: finished) {
            AssetLoadResult asyncReturn = p_PendingLoadTasks[handle].get();
            // enqueue any new loads
            for (auto& newLoad: asyncReturn.m_NewAssetTasks) {
                LoadAsset(newLoad.path, newLoad.type);
            }

            if (asyncReturn.m_SyncAssetCallbacks.empty()
                && std::holds_alternative<AssetTransientData*>(asyncReturn.m_Next)) {
                auto* transient = std::get<AssetTransientData*>(asyncReturn.m_Next);
                TransitionAssetToLoaded(handle, transient->m_AssetDataPtr);

                asyncReturn.m_Next = transient->m_AssetDataPtr;

                AXM_DELETE(transient);
            } else {
                TransitionAssetToLoaded(handle, std::get<Asset*>(asyncReturn.m_Next));
            }

            p_PendingLoadTasks.erase(handle);
        }
    }

    void AssetManager::DispatchAssetLoadTask(const AssetHandle& handle, AssetLoadInfo& info) {
        PROFILE_SCOPE();
        if (p_AssetFactories.find(handle.m_AssetType) == p_AssetFactories.end()) {
            return;
        }
        p_PendingLoadTasks.emplace(
                handle,
                std::move(std::async(std::launch::async, p_AssetFactories[handle.m_AssetType].first, info.path)));
    }

    void AssetManager::TransitionAssetToLoaded(const AssetHandle& handle, Asset* asset_to_transition) {
        PROFILE_SCOPE();
        p_LoadedAssets.emplace(handle, std::move(Unique<Asset>(asset_to_transition)));

        // TODO: Log Asset Loaded
        if (p_AssetLoadCallbacks.find(handle) == p_AssetLoadCallbacks.end()) {
            AXM_LOG("Asset {} already loaded", asset_to_transition->m_Path);
            return;
        }

        for (auto& [ah, loaded_callback]: p_AssetLoadCallbacks) {
            loaded_callback(p_LoadedAssets[ah].get());
        }

        p_AssetLoadCallbacks.erase(handle);
    }
} // namespace axm
