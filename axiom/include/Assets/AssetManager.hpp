#pragma once
#include "Assets/Assets.hpp"
#include "Core/Prim.hpp"

namespace axm {

    struct AssetLoadInfo
    {
        String path;
        AssetType type;

        bool operator==(const AssetLoadInfo& o) const { return path == o.path && type == o.type; }

        AssetLoadInfo& operator=(const AssetLoadInfo& o) = default;

        bool operator<(const AssetLoadInfo& o) const { return path.size() < o.path.size(); }

        NO_DISCARD AssetHandle ToHandle() const;
    };

    enum class AssetLoadProgress { NotLoaded, Loading, Loaded, Unloading };

    /// <summary>
    /// Asset callback type defs
    /// </summary>
    using AssetIntermediateCallback = void (*)(AssetTransientData*);
    using OnAssetLoadedCallback = void (*)(Asset*);
    using OnAssetUnloadedCallback = void (*)(Asset*);

    struct AssetLoadResult
    {
        // Asset may return an intermediate _or_ the full asset.
        Variant<Asset*, AssetTransientData*> m_Next;
        // additional assets that may be required to completely load this asset
        Vector<AssetLoadInfo> m_NewAssetTasks;
        // synchronous tasks associated with this asset e.g. submit tex mem to GPu
        Vector<AssetIntermediateCallback> m_SyncAssetCallbacks;
    };

    using LoadAssetCallback = AssetLoadResult (*)(const String& path);
    using UnloadAssetCallback = void (*)(Asset* a);

    class AssetManager
    {
    public:
        AssetManager() = default;

        bool ProvideAssetFactory(const AssetType& type, LoadAssetCallback onLoad, UnloadAssetCallback onUnload);

        AssetHandle
        LoadAsset(const String& path, const AssetType& assetType, OnAssetLoadedCallback onAssetLoaded = nullptr);

        void UnloadAsset(const AssetHandle& handle);
        Asset* GetAsset(const AssetHandle& handle);

        template <typename AssetType>
        AssetType* GetAsset(const AssetHandle& handle) {
            static_assert(std::is_base_of<Asset, AssetType>() && "Provided type is not an asset");
            auto* a = GetAsset(handle);
            if (a) {
                return static_cast<AssetType*>(a);
            }
            return nullptr;
        }

        template <typename AssetType>
        AssetType* GetAsset(const String& path) {
            static_assert(std::is_base_of<Asset, AssetType>() && "Provided type is not an asset");

            auto handle = AssetHandle(path, AssetType::kAssetEnumType);
            auto* a = GetAsset(handle);
            if (a) {
                return static_cast<AssetType*>(a);
            }
            return nullptr;
        }
        AssetLoadProgress GetAssetLoadProgress(const AssetHandle& handle);

        NO_DISCARD bool AnyAssetsLoading() const;
        NO_DISCARD bool AnyAssetsUnloading() const;

        /// <summary>
        /// Synchronous calls that will wait until
        /// all pending asset manager loads /unloads are finished
        /// </summary>
        void WaitAllAssets();
        void WaitAllUnloads();
        void UnloadAllAssets();

        void Update();
        void Shutdown();

    protected:
        friend struct Engine;
        HashMap<AssetType, Pair<LoadAssetCallback, UnloadAssetCallback>> p_AssetFactories;
        HashMap<AssetHandle, Future<AssetLoadResult>> p_PendingLoadTasks;
        HashMap<AssetHandle, Unique<Asset>> p_LoadedAssets;
        HashMap<AssetHandle, AssetLoadResult> p_PendingSyncCallbacks;
        HashMap<AssetHandle, OnAssetUnloadedCallback> p_PendingUnloadCallbacks;
        HashMap<AssetHandle, OnAssetLoadedCallback> p_AssetLoadCallbacks;
        Vector<AssetLoadInfo> p_QueuedLoads;

        static constexpr u16 kCallbackTasksPerUpdate = 1;
        static constexpr u16 kMaxAsyncTasksInFlight = 8;

        void HandleCallbacks();

        void HandlePendingLoads();

        void HandleAsyncTasks();

        void DispatchAssetLoadTask(const AssetHandle& handle, AssetLoadInfo& info);

    private:
        void TransitionAssetToLoaded(const AssetHandle& handle, Asset* asset_to_transition);
    };
} // namespace axm
