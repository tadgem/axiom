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

  AssetHandle to_handle();
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
  Vector<AssetLoadInfo> new_asset_tasks;
  // synchronous tasks associated with this asset e.g. submit tex mem to GPU
  // in openGL
  Vector<AssetIntermediateCallback> sync_asset_callbacks;
};

using LoadAssetCallback = AssetLoadResult (*)(const String &path);
using UnloadAssetCallback = void (*)(Asset *a);

class AssetManager {
public:
  AssetManager() = default;

  bool provide_asset_factory(const AssetType &type,
                                    LoadAssetCallback onLoad,
                                    UnloadAssetCallback onUnload);

  AssetHandle load_asset(const String &path, const AssetType &assetType,
                        OnAssetLoadedCallback onAssetLoaded = nullptr);

  void unload_asset(const AssetHandle &handle);
  Asset *get_asset(const AssetHandle &handle);

  template <typename _Ty> _Ty *get_asset(const AssetHandle &handle) {
    static_assert(std::is_base_of<Asset, _Ty>() &&
                  "Provided type is not an asset");
    auto *a = get_asset(handle);
    if (a) {
      return static_cast<_Ty *>(a);
    }
    return nullptr;
  }
  AssetLoadProgress get_asset_load_progress(const AssetHandle &handle);

  bool any_assets_loading();
  bool any_assets_unloading();

  /// <summary>
  /// Synchronous calls that will wait until
  /// all pending asset manager loads /unloads are finished
  /// </summary>
  void wait_all_assets();
  void wait_all_unloads();
  void unload_all_assets();

  void update();
  void shutdown();

protected:
  friend struct Engine;
  HashMap<AssetType, Pair<LoadAssetCallback, UnloadAssetCallback>>  asset_factories;
  HashMap<AssetHandle, Future<AssetLoadResult>>                     pending_load_tasks;
  HashMap<AssetHandle, Unique<Asset>>                               loaded_assets;
  HashMap<AssetHandle, AssetLoadResult>                             pending_sync_callbacks;
  HashMap<AssetHandle, OnAssetUnloadedCallback>                     pending_unload_callbacks;
  HashMap<AssetHandle, OnAssetLoadedCallback>                       asset_loaded_callbacks;
  Vector<AssetLoadInfo>                                             queued_loads;

  static constexpr u16 kCallbackTasksPerUpdate = 1;
  static constexpr u16 kMaxAsyncTasksInFlight = 8;

  void handle_callbacks();

  void handle_pending_loads();

  void handle_async_tasks();

  void dispatch_asset_load_task(const AssetHandle &handle, AssetLoadInfo &info);

private:
  void transition_asset_to_loaded(const AssetHandle &handle,
                               Asset *asset_to_transition);
};
} // namespace harmony
