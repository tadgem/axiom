#include "AssetManager.hpp"

namespace axm {
AssetHandle AssetLoadInfo::ToHandle() { return AssetHandle(path, type); }

bool AssetManager::ProvideAssetFactory(const AssetType &type,
                                                LoadAssetCallback onLoad,
                                                UnloadAssetCallback onUnload) {
  if (p_AssetFactories.find(type) != p_AssetFactories.end()) {
    return false;
  }

  Pair<LoadAssetCallback, UnloadAssetCallback> funcs{onLoad, onUnload};

  p_AssetFactories[type] = funcs;
  return true;
}

AssetHandle AssetManager::LoadAsset(const String &path,
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
  AssetLoadInfo loadInfo{tmp_path, assetType};

  auto it = std::find(p_QueuedLoads.begin(), p_QueuedLoads.end(), loadInfo);

  for (auto &queued_load : p_QueuedLoads) {
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

void AssetManager::UnloadAsset(const AssetHandle &handle) {
  // Asset is not loaded
  if (p_LoadedAssets.find(handle) == p_LoadedAssets.end()) {
    return;
  }

  // make sure we have a provided function to unload the asset
  if (p_AssetFactories.find(handle.type) == p_AssetFactories.end()) {
    return;
  }

  // Enqueue unload
  p_PendingUnloadCallbacks.emplace(handle,
                                  p_AssetFactories[handle.type].second);
}

Asset *AssetManager::GetAsset(const AssetHandle &handle) {
  if (p_LoadedAssets.find(handle) == p_LoadedAssets.end()) {
    return nullptr;
  }

  return p_LoadedAssets[handle].get();
}

AssetLoadProgress
AssetManager::GetAssetLoadProgress(const AssetHandle &handle) {
  for (auto &queued : p_QueuedLoads) {
    if (queued.ToHandle() == handle) {
      return AssetLoadProgress::Loading;
    }
  }

  if (p_PendingLoadTasks.find(handle) != p_PendingLoadTasks.end() ||
      p_PendingSyncCallbacks.find(handle) !=
          p_PendingSyncCallbacks.end()) {
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
  return !p_PendingLoadTasks.empty() || !p_PendingSyncCallbacks.empty() ||
         !p_PendingUnloadCallbacks.empty() || !p_QueuedLoads.empty();
}

bool AssetManager::AnyAssetsUnloading() const {
  return !p_PendingUnloadCallbacks.empty();
}

void AssetManager::WaitAllAssets() {
  while (AnyAssetsLoading()) {
    Update();
  }
}

void AssetManager::WaitAllUnloads() {
  while (AnyAssetsUnloading()) {
    Update();
  }
}

void AssetManager::UnloadAllAssets() {
  Vector<AssetHandle> assetsRemaining{};

  for (auto &[handle, asset] : p_LoadedAssets) {
    assetsRemaining.push_back(handle);
  }

  for (auto &handle : assetsRemaining) {
    UnloadAsset(handle);
  }

  WaitAllUnloads();
}

void AssetManager::Update() {
  if (!AnyAssetsLoading()) {
    return;
  }

  HandleCallbacks();
  HandlePendingLoads();
  HandleAsyncTasks();
}

void AssetManager::Shutdown() {
  WaitAllAssets();
  WaitAllUnloads();
  UnloadAllAssets();
}

void AssetManager::HandleCallbacks() {
  u16 processedCallbacks = 0;
  Vector<AssetHandle> clears;

  for (auto &[handle, asset] : p_PendingSyncCallbacks) {
    if (processedCallbacks == kCallbackTasksPerUpdate)
      break;

    for (u16 i = 0; i < kCallbackTasksPerUpdate - processedCallbacks; i++) {
      if (i >= asset.m_SyncAssetCallbacks.size())
        break;
      asset.m_SyncAssetCallbacks.back()(asset.m_TransientAssetData);
      asset.m_SyncAssetCallbacks.pop_back();
      processedCallbacks++;
    }

    if (asset.m_SyncAssetCallbacks.empty()) {
      clears.push_back(handle);
    }
  }
  for (auto &handle : clears) {
    TransitionAssetToLoaded(
        handle,
        p_PendingSyncCallbacks[handle].m_TransientAssetData->m_AssetDataPtr);
    AXM_DELETE(p_PendingSyncCallbacks[handle].m_TransientAssetData);
    p_PendingSyncCallbacks.erase(handle);
  }
  clears.clear();

  for (auto &[handle, callback] : p_PendingUnloadCallbacks) {
    if (processedCallbacks == kCallbackTasksPerUpdate)
      break;
    callback(p_LoadedAssets[handle].get());
    clears.push_back(handle);
    processedCallbacks++;
  }

  for (auto &handle : clears) {
    p_PendingUnloadCallbacks.erase(handle);
    p_LoadedAssets[handle].reset();
    p_LoadedAssets.erase(handle);
  }
}

void AssetManager::HandlePendingLoads() {
  while (p_PendingLoadTasks.size() <= kMaxAsyncTasksInFlight &&
         !p_QueuedLoads.empty()) {
    auto &info = p_QueuedLoads.front();
    DispatchAssetLoadTask(info.ToHandle(), info);
    p_QueuedLoads.erase(p_QueuedLoads.begin());
  }
}

void AssetManager::HandleAsyncTasks() {
  Vector<AssetHandle> finished;
  for (auto &[handle, future] : p_PendingLoadTasks) {
    if (IsFutureReady(future)) {
      finished.push_back(handle);
    }
  }

  for (auto &handle : finished) {
    AssetLoadResult asyncReturn = p_PendingLoadTasks[handle].get();
    // enqueue new loads
    for (auto &newLoad : asyncReturn.m_NewAssetTasks) {
      LoadAsset(newLoad.path, newLoad.type);
    }

    if (asyncReturn.m_SyncAssetCallbacks.empty() &&
        asyncReturn.m_TransientAssetData != nullptr) {
      TransitionAssetToLoaded(handle,
                              asyncReturn.m_TransientAssetData->m_AssetDataPtr);

      AXM_DELETE(asyncReturn.m_TransientAssetData);
      asyncReturn.m_TransientAssetData = nullptr;
    } else {
      p_PendingSyncCallbacks.emplace(handle, asyncReturn);
    }

    p_PendingLoadTasks.erase(handle);
  }
}

void AssetManager::DispatchAssetLoadTask(const AssetHandle &handle,
                                         AssetLoadInfo &info) {
  if (p_AssetFactories.find(handle.type) == p_AssetFactories.end()) {
    return;
  }
  p_PendingLoadTasks.emplace(
      handle, std::move(std::async(std::launch::async,
                                   p_AssetFactories[handle.type].first,
                                   info.path)));
}

void AssetManager::TransitionAssetToLoaded(const AssetHandle &handle,
                                           Asset *asset_to_transition) {
  p_LoadedAssets.emplace(handle, std::move(Unique<Asset>(asset_to_transition)));

  // TODO: Log Asset Loaded
  if (p_AssetLoadCallbacks.find(handle) == p_AssetLoadCallbacks.end()) {
    return;
  }

  for (auto &[ah, loaded_callback] : p_AssetLoadCallbacks) {
    loaded_callback(p_LoadedAssets[ah].get());
  }

  p_AssetLoadCallbacks.erase(handle);
}
} // namespace harmony
