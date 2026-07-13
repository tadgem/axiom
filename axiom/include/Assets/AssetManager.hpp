#pragma once
#include "Assets/Assets.hpp"
#include "Core/Prim.hpp"

namespace axm {

    /// <summary>
    /// Asset callback type defs
    /// </summary>
    using InterFn      = void (*)(AssetTransient*);
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

    enum class AssetState { NotLoaded, Loading, Loaded, Unloading };

    struct AssetErrorMessage
    {
        String m_Message;
    };


    struct AssetLoadResult
    {
        Variant<Asset*, AssetTransient*, AssetErrorMessage>
                              m_Next; // may be final asset or transient while factory processes N steps
        Vector<AssetLoadInfo> m_NewAssetTasks; // new assets to be loaded (e.g. model requests new tex)
    };

    class AssetFactory
    {
    public:
        const AssetType m_AssetType;

        AssetFactory(AssetType assetType) : m_AssetType(assetType) { };

        NO_DISCARD virtual AssetLoadResult LoadAsset(const String& path) const = 0;
        virtual void                       UnloadAsset(Asset* asset) const     = 0;
        virtual void                       ProcessAssetTransient(AssetTransient* data) const { };

        virtual ~AssetFactory() = default;
    };

    class AssetManager
    {
    public:
        AssetManager() = default;

        template <AssetType AssetTypeEnum, typename T, typename... Args>
        T* AddAssetFactory(Args&&... args) {
            static_assert(std::is_base_of<AssetFactory, T>() && "Provided type is not an AssetFactory");
            if (p_AssetFactories.contains(AssetTypeEnum)) {
                AXM_LOG("Asset Manager : Already have a provided factory for type {}", typeid(AssetTypeEnum).name());
                return static_cast<T*>(p_AssetFactories[AssetTypeEnum].get());
            }

            T* f = AXM_NEW(T, std::forward<Args>(args)...);
            p_AssetFactories.emplace(AssetTypeEnum, Unique<AssetFactory>(f));
            return static_cast<T*>(p_AssetFactories[AssetTypeEnum].get());
        }

        AssetHandle LoadAsset(const String& path, const AssetType& assetType, const OnLoadedFn& onLoaded = nullptr);
        void        UnloadAsset(const AssetHandle& handle);
        Asset*      GetAsset(const AssetHandle& handle);

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

        AssetState      GetAssetLoadProgress(const AssetHandle& handle);

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

        struct AssetTransientData
        {
            Unique<AssetTransient> m_Transient;
            OnLoadedFn             m_LoadCallback = nullptr;
        };

        struct AsyncAssetData
        {
            String                  m_Path;
            AssetType               m_AssetType;
            Future<AssetLoadResult> m_Task;
            OnLoadedFn              m_LoadCallback;
        };

        HashMap<AssetType, Unique<AssetFactory>> p_AssetFactories;
        HashMap<AssetHandle, Unique<Asset>>      p_LoadedAssets;
        Vector<AssetHandle>                      p_QueuedUnloads;
        Vector<AssetLoadInfo>                    p_QueuedLoads;

        Vector<AsyncAssetData>                   p_InFlightLoads;
        Vector<AssetTransientData>               p_InFlightTransients;

        static constexpr u16                     kMaxMainThreadTasksPerTick = 1;
        static constexpr u16                     kMaxAsyncTasksPerTick      = 1;

        void                                     HandlePendingLoads(u16& remainingTasks);
        void                                     HandleAsyncTasks(u16& remainingTasks);
        void                                     HandleTransients(u16& remainingTasks);
        void                                     HandleUnloads(u16& remainingTasks);
        void                                     TransitionAssetToLoaded(Asset* asset, OnLoadedFn loadCallback);
    };
} // namespace axm
