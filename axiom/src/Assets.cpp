
#include "Assets.hpp"

namespace axm {
    SerializableAssetHandle::SerializableAssetHandle(const String &p, const AssetType &type) :
        m_Path(p), m_Handle(p, type) {}

    AssetHandle::AssetHandle() : m_AssetType(AssetType::Unknown), m_PathHash(UINT64_MAX) {}

    AssetHandle::AssetHandle(const String &p, const AssetType &type) : m_PathHash(HashString(p)), m_AssetType(type) {}

    Asset::Asset(const String &path, const AssetType &type) : m_Path(path), m_Handle(path, type) {}

} // namespace axm
