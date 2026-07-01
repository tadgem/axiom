#pragma once
#include "Assets.hpp"
#include "Prim.hpp"

namespace axm {

struct AssetLoadInfo {
  String    path;
  AssetType type;

  bool operator==(const AssetLoadInfo &o) const {
    return path == o.path && type == o.type;
  }

  void operator=(const AssetLoadInfo &o) {
    path = o.path;
    type = o.type;
  }

  bool operator<(const AssetLoadInfo &o) const {
    return path.size() < o.path.size();
  }

  AssetHandle ToHandle();
};

enum class AssetLoadProgress { NotLoaded, Loading, Loaded, Unloading };

/// <summary>
/// Asset callback type defs
/// </summary>
using AssetIntermediateCallback = void (*)(AssetTransientData *);
using OnAssetLoadedCallback = void (*)(Asset *);
using OnAssetUnloadedCallback = void (*)(Asset *);

struct AssetLoadResult {
  AssetTransientData *loaded_intermediate = nullptr;
  // additional assets that may be required to completely load this asset
  Vector<AssetLoadInfo> NewAssetTasks;
  // synchronous tasks associated with this asset e.g. submit tex mem to GPU
  Vector<AssetIntermediateCallback> SyncAssetCallbacks;
};

using LoadAssetCallback = AssetLoadResult (*)(const String &path);
using UnloadAssetCallback = void (*)(Asset *a);

class AssetManager {
public:
  AssetManager() = default;

  bool ProvideAssetFactory(const AssetType &type,
                                    LoadAssetCallback onLoad,
                                    UnloadAssetCallback onUnload);

  AssetHandle LoadAsset(const String &path, const AssetType &assetType,
                        OnAssetLoadedCallback onAssetLoaded = nullptr);

  void UnloadAsset(const AssetHandle &handle);
  Asset *GetAsset(const AssetHandle &handle);

  template <typename _Ty> _Ty *GetAsset(const AssetHandle &handle) {
    static_assert(std::is_base_of<Asset, _Ty>() &&
                  "Provided type is not an asset");
    auto *a = GetAsset(handle);
    if (a) {
      return static_cast<_Ty *>(a);
    }
    return nullptr;
  }
  AssetLoadProgress GetAssetLoadProgress(const AssetHandle &handle);

  bool AnyAssetsLoading();
  bool AnyAssetsUnloading();

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
  HashMap<AssetType, Pair<LoadAssetCallback, UnloadAssetCallback>>  p_AssetFactories;
  HashMap<AssetHandle, Future<AssetLoadResult>>                     p_PendingLoadTasks;
  HashMap<AssetHandle, Unique<Asset>>                               p_LoadedAssets;
  HashMap<AssetHandle, AssetLoadResult>                             p_PendingSyncCallbacks;
  HashMap<AssetHandle, OnAssetUnloadedCallback>                     p_PendingUnloadCallbacks;
  HashMap<AssetHandle, OnAssetLoadedCallback>                       p_AssetLoadCallbacks;
  Vector<AssetLoadInfo>                                             p_QueuedLoads;

  static constexpr u16 kCallbackTasksPerUpdate = 1;
  static constexpr u16 kMaxAsyncTasksInFlight = 8;

  void HandleCallbacks();

  void HandlePendingLoads();

  void HandleAsyncTasks();

  void DispatchAssetLoadTask(const AssetHandle &handle, AssetLoadInfo &info);

private:
  void TransitionAssetToLoaded(const AssetHandle &handle,
                               Asset *asset_to_transition);
};
} // namespace axm
