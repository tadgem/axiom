
#include "Assets/Assets.hpp"


namespace axm {

    AssetHandle AssetHandle::BAD = AssetHandle(0, AssetType::Unknown);

    SerializableAssetHandle::SerializableAssetHandle(const String& p, const AssetType& type) :
        m_Path(p), m_Handle(p, type) { }

    AssetHandle::AssetHandle() : m_AssetType(AssetType::Unknown), m_PathHash(UINT64_MAX) { }
    AssetHandle::AssetHandle(const Filesystem::path& path, const AssetType& type) :
        m_PathHash(HashPath(path)), m_AssetType(type) { }

    AssetHandle::AssetHandle(const String& p, const AssetType& type) : m_PathHash(HashString(p)), m_AssetType(type) { }

    AssetHandle::AssetHandle(const str_hash& hash, const AssetType& type) : m_PathHash(hash), m_AssetType(type) { }

    Asset::Asset(const Filesystem::path& path, const AssetType& type) : m_Path(path), m_Handle(path, type) { }

} // namespace axm
