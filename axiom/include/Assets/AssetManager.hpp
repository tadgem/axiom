#pragma once
#include "Assets/Assets.hpp"
#include "Core/Prim.hpp"

#define AXM_HAS_TRANSIENT true

namespace axm {

    /// <summary>
    /// Asset callback type defs
    /// </summary>
    using InterFn      = void (*)(AssetTransientData*);
    using OnLoadedFn   = Function<void, Asset*>;
    using OnUnloadedFn = void (*)(Asset*);

    struct AssetLoadInfo
    {
        String     m_Path;
        AssetType  m_AssetType;
        OnLoadedFn m_OnLoadedCallback;


        bool operator==(const AssetLoadInfo& o) const { return m_Path == o.m_Path && m_AssetType == o.m_AssetType; }
        AssetLoadInfo&         operator=(const AssetLoadInfo& o) = default;
        bool                   operator<(const AssetLoadInfo& o) const { return m_Path.size() < o.m_Path.size(); }

        NO_DISCARD AssetHandle ToHandle() const;
    };

    enum class AssetLoadProgress { NotLoaded, Loading, Loaded, Unloading };


    struct AssetLoadResult
    {
        Variant<Asset*, AssetTransientData*> m_Next; // may be final asset or transient while factory processes N steps
        Vector<AssetLoadInfo>                m_NewAssetTasks; // new assets to be loaded (e.g. model requests new tex)
        u16                                  m_TransientSteps; // number of steps for transient asset in factory
    };

    using LoadAssetCallback   = AssetLoadResult (*)(const String& path);
    using UnloadAssetCallback = void (*)(Asset* a);

    class AssetFactory
    {
    public:
        const AssetType m_AssetType;
        const bool      m_HasTransient;

        AssetFactory(AssetType assetType, bool hasTransient) : m_AssetType(assetType), m_HasTransient(hasTransient) { };

        virtual AssetLoadResult LoadAsset(const String& path) const = 0;
        virtual void            UnloadAsset(Asset* asset) const     = 0;
        virtual void            ProcessAssetTransient(AssetTransientData* data, u16 step) const { };

        virtual ~AssetFactory() = default;
    };

    class AssetManager
    {
    public:
        AssetManager() = default;

        template <AssetType AssetTypeEnum, typename T, typename... Args>
        bool AddAssetFactory(Args&&... args) {
            static_assert(std::is_base_of<AssetFactory, T>() && "Provided type is not an AssetFactory");
            if (p_AssetFactories.contains(AssetTypeEnum)) {
                AXM_LOG("Asset Manager : Already have a provided factory for type {}", typeid(AssetTypeEnum).name());
                return false;
            }

            p_AssetFactories.emplace(AssetTypeEnum, MakeUnique<T>(std::forward<Args>(args)...));
            return true;
        }

        AssetHandle
               LoadAsset(const String& path, const AssetType& assetType, const OnLoadedFn& onAssetLoaded = nullptr);
        void   UnloadAsset(const AssetHandle& handle);
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

            auto  handle = AssetHandle(path, AssetType::kAssetEnumType);
            auto* a      = GetAsset(handle);
            if (a) {
                return static_cast<AssetType*>(a);
            }
            return nullptr;
        }
        AssetLoadProgress GetAssetLoadProgress(const AssetHandle& handle);

        NO_DISCARD bool   AnyAssetsLoading() const;
        NO_DISCARD bool   AnyAssetsUnloading() const;

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
        HashMap<AssetType, Unique<AssetFactory>>      p_AssetFactories;
        HashMap<AssetHandle, Future<AssetLoadResult>> p_PendingLoadTasks;
        HashMap<AssetHandle, Unique<Asset>>           p_LoadedAssets;
        HashMap<AssetHandle, AssetLoadResult>         p_PendingSyncCallbacks;
        Vector<AssetHandle>                           p_PendingUnloads;
        Vector<AssetHandle>                           p_InProgressAssetLoads;
        Vector<AssetLoadInfo>                         p_QueuedLoads;

        static constexpr u16                          kCallbackTasksPerUpdate = 1;
        static constexpr u16                          kMaxAsyncTasksInFlight  = 8;

        void                                          HandleCallbacks();

        void                                          HandlePendingLoads();

        void                                          HandleAsyncTasks();

        void DispatchAssetLoadTask(const AssetHandle& handle, AssetLoadInfo& info);

    private:
        void TransitionAssetToLoaded(const AssetHandle& handle, Asset* asset_to_transition);
    };
} // namespace axm
