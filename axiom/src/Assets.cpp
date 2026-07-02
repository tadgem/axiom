
#include "Assets.hpp"

namespace axm {
    SerializableAssetHandle::SerializableAssetHandle(const String &p,
                                                     const AssetType &type)
        : path(p), handle(p, type) {}

    AssetHandle::AssetHandle() : type(AssetType::Unknown), path_hash(UINT64_MAX) {}

    AssetHandle::AssetHandle(const String &p, const AssetType &type)
        : path_hash(HashString(p)), type(type) {}

    Asset::Asset(const String &path, const AssetType &type)
        : path(path), handle(path, type) {}

} // namespace axm
