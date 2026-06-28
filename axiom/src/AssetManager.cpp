#include "AssetManager.hpp"

namespace axm {
AssetHandle AssetLoadInfo::to_handle() { return AssetHandle(path, type); }

bool AssetManager::provide_asset_factory(const AssetType &type,
                                                LoadAssetCallback onLoad,
                                                UnloadAssetCallback onUnload) {
  if (asset_factories.find(type) != asset_factories.end()) {
    return false;
  }

  Pair<LoadAssetCallback, UnloadAssetCallback> funcs{onLoad, onUnload};

  asset_factories[type] = funcs;
  return true;
}

AssetHandle AssetManager::load_asset(const String &path,
                                    const AssetType &assetType,
                                    OnAssetLoadedCallback onAssetLoaded) {
  if (!Filesystem::exists(path.c_str())) {
    return {};
  }

  String wd(Filesystem::current_path().string().c_str());
  String tmp_path = path;
  if (path.find(wd) != std::string::npos) {
    tmp_path.erase(tmp_path.find(wd), wd.length());

    for (int i = 0; i < 2; i++) {
      if (tmp_path[0] != '\\' && tmp_path[0] != '/') {
        break;
      }
      tmp_path.erase(0, 1);
    }
  }

  AssetHandle handle(tmp_path, assetType);
  AssetLoadInfo load_info{tmp_path, assetType};

  auto it = std::find(queued_loads.begin(), queued_loads.end(), load_info);

  for (auto &queued_load : queued_loads) {
    if (load_info == queued_load) {
      return queued_load.to_handle();
    }
  }

  queued_loads.push_back(load_info);

  if (onAssetLoaded != nullptr) {
    asset_loaded_callbacks.emplace(handle, onAssetLoaded);
  }

  return handle;
}

void AssetManager::unload_asset(const AssetHandle &handle) {
  // Asset is not loaded
  if (loaded_assets.find(handle) == loaded_assets.end()) {
    return;
  }

  // make sure we have a provided function to unload the asset
  if (asset_factories.find(handle.type) == asset_factories.end()) {
    return;
  }

  // Enqueue unload
  pending_unload_callbacks.emplace(handle,
                                  asset_factories[handle.type].second);
}

Asset *AssetManager::get_asset(const AssetHandle &handle) {
  if (loaded_assets.find(handle) == loaded_assets.end()) {
    return nullptr;
  }

  return loaded_assets[handle].get();
}

AssetLoadProgress
AssetManager::get_asset_load_progress(const AssetHandle &handle) {
  for (auto &queued : queued_loads) {
    if (queued.to_handle() == handle) {
      return AssetLoadProgress::Loading;
    }
  }

  if (pending_load_tasks.find(handle) != pending_load_tasks.end() ||
      pending_sync_callbacks.find(handle) !=
          pending_sync_callbacks.end()) {
    return AssetLoadProgress::Loading;
  }

  if (pending_unload_callbacks.find(handle) != pending_unload_callbacks.end()) {
    return AssetLoadProgress::Unloading;
  }

  if (loaded_assets.find(handle) != loaded_assets.end()) {
    return AssetLoadProgress::Loaded;
  }

  return AssetLoadProgress::NotLoaded;
}

bool AssetManager::any_assets_loading() {
  return !pending_load_tasks.empty() || !pending_sync_callbacks.empty() ||
         !pending_unload_callbacks.empty() || !queued_loads.empty();
}

bool AssetManager::any_assets_unloading() {
  return !pending_unload_callbacks.empty();
}

void AssetManager::wait_all_assets() {
  while (any_assets_loading()) {
    update();
  }
}

void AssetManager::wait_all_unloads() {
  while (any_assets_unloading()) {
    update();
  }
}

void AssetManager::unload_all_assets() {
  Vector<AssetHandle> assetsRemaining{};

  for (auto &[handle, asset] : loaded_assets) {
    assetsRemaining.push_back(handle);
  }

  for (auto &handle : assetsRemaining) {
    unload_asset(handle);
  }

  wait_all_unloads();
}

void AssetManager::update() {
  if (!any_assets_loading()) {
    return;
  }

  handle_callbacks();
  handle_pending_loads();
  handle_async_tasks();
}

void AssetManager::shutdown() {
  wait_all_assets();
  wait_all_unloads();
  unload_all_assets();
}

void AssetManager::handle_callbacks() {
  u16 processedCallbacks = 0;
  Vector<AssetHandle> clears;

  for (auto &[handle, asset] : pending_sync_callbacks) {
    if (processedCallbacks == kCallbackTasksPerUpdate)
      break;

    for (u16 i = 0; i < kCallbackTasksPerUpdate - processedCallbacks; i++) {
      if (i >= asset.sync_asset_callbacks.size())
        break;
      asset.sync_asset_callbacks.back()(asset.loaded_intermediate);
      asset.sync_asset_callbacks.pop_back();
      processedCallbacks++;
    }

    if (asset.sync_asset_callbacks.empty()) {
      clears.push_back(handle);
    }
  }
  for (auto &handle : clears) {
    transition_asset_to_loaded(
        handle,
        pending_sync_callbacks[handle].loaded_intermediate->asset_data_ptr);
    AXM_DELETE(pending_sync_callbacks[handle].loaded_intermediate);
    pending_sync_callbacks.erase(handle);
  }
  clears.clear();

  for (auto &[handle, callback] : pending_unload_callbacks) {
    if (processedCallbacks == kCallbackTasksPerUpdate)
      break;
    callback(loaded_assets[handle].get());
    clears.push_back(handle);
    processedCallbacks++;
  }

  for (auto &handle : clears) {
    pending_unload_callbacks.erase(handle);
    loaded_assets[handle].reset();
    loaded_assets.erase(handle);
  }
}

void AssetManager::handle_pending_loads() {
  while (pending_load_tasks.size() <= kMaxAsyncTasksInFlight &&
         !queued_loads.empty()) {
    auto &info = queued_loads.front();
    dispatch_asset_load_task(info.to_handle(), info);
    queued_loads.erase(queued_loads.begin());
  }
}

void AssetManager::handle_async_tasks() {
  Vector<AssetHandle> finished;
  for (auto &[handle, future] : pending_load_tasks) {
    if (IsFutureReady(future)) {
      finished.push_back(handle);
    }
  }

  for (auto &handle : finished) {
    AssetLoadResult asyncReturn = pending_load_tasks[handle].get();
    // enqueue new loads
    for (auto &newLoad : asyncReturn.new_asset_tasks) {
      load_asset(newLoad.path, newLoad.type);
    }

    if (asyncReturn.sync_asset_callbacks.empty() &&
        asyncReturn.loaded_intermediate != nullptr) {
      transition_asset_to_loaded(handle,
                              asyncReturn.loaded_intermediate->asset_data_ptr);

      AXM_DELETE(asyncReturn.loaded_intermediate);
      asyncReturn.loaded_intermediate = nullptr;
    } else {
      pending_sync_callbacks.emplace(handle, asyncReturn);
    }

    pending_load_tasks.erase(handle);
  }
}

void AssetManager::dispatch_asset_load_task(const AssetHandle &handle,
                                         AssetLoadInfo &info) {
  if (asset_factories.find(handle.type) == asset_factories.end()) {
    return;
  }
  pending_load_tasks.emplace(
      handle, std::move(std::async(std::launch::async,
                                   asset_factories[handle.type].first,
                                   info.path)));
}

void AssetManager::transition_asset_to_loaded(const AssetHandle &handle,
                                           Asset *asset_to_transition) {
  loaded_assets.emplace(handle, std::move(Unique<Asset>(asset_to_transition)));

  // TODO: Log Asset Loaded
  if (asset_loaded_callbacks.find(handle) == asset_loaded_callbacks.end()) {
    return;
  }

  for (auto &[ah, loaded_callback] : asset_loaded_callbacks) {
    loaded_callback(loaded_assets[ah].get());
  }

  asset_loaded_callbacks.erase(handle);
}
} // namespace harmony
