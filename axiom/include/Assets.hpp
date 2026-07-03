#pragma once
#include "Prim.hpp"
#include "STL.hpp"

namespace axm {
    enum class AssetType : u8 { Unknown, Model, Texture, Shader, Audio, Text, Binary };

    /// <summary>
    /// Lightweight identifier for an arbitrary asset
    /// use in collections for fast look up
    /// </summary>
    struct AssetHandle {
        AssetType type;
        str_hash path_hash;

        AssetHandle();
        AssetHandle(const String &p, const AssetType &type);

        bool operator==(const AssetHandle &o) const { return type == o.type && path_hash == o.path_hash; }

        bool operator<(const AssetHandle &o) const { return type < o.type && path_hash < o.path_hash; }
    };

    /// <summary>
    /// Allow asset to be loaded by asset manager system
    /// Giving concrete path and type of asset
    /// </summary>
    struct SerializableAssetHandle {
        AssetHandle handle;
        String path;

        SerializableAssetHandle(const String &p, const AssetType &type);
    };

    class Asset {
    public:
        Asset(const String &path, const AssetType &type);
        virtual ~Asset() {};

        const String path;
        const AssetHandle handle;
    };

    template<typename _Ty, AssetType _AssetTypeEnum>
    class AssetT : public Asset {
    public:
        _Ty data;

        AssetT(const String &path, const _Ty &data) : Asset(path, _AssetTypeEnum), data(data) {};

        ~AssetT() override {};
    };

    /// <summary>
    /// Allows intermediate data for assets that require submission in multiple
    /// phases e.g. load texture from disk to ram -> transfer from ram to GPU Once
    /// AssetIntermediate has finished, the asset is moved to the ready state.
    /// </summary>
    class AssetTransientData {
    public:
        Asset *m_AssetDataPtr;
        AssetTransientData(Asset *asset) : m_AssetDataPtr(asset) {}

        virtual ~AssetTransientData() {}
    };

    template<typename _AssetTy, typename _IntermediateType, AssetType _AssetTypeEnum>
    class AssetTransientT : public AssetTransientData {
    public:
        _IntermediateType m_TransientData;

        AssetTransientT(Asset *data, const _IntermediateType &inter) :
            AssetTransientData(data), m_TransientData(inter) {}

        AssetT<_AssetTy, _AssetTypeEnum> get_concrete_asset() {
            return static_cast<AssetT<_AssetTy, _AssetTypeEnum>>(m_AssetDataPtr);
        }
    };

} // namespace axm

/// <summary>
/// Allow AssetHandle to be used as a key in unordered_map
/// </summary>
template<>
struct std::hash<axm::AssetHandle> {
    size_t operator()(const axm::AssetHandle &ah) const {
        return std::hash<i64>()(ah.path_hash) ^ std::hash<u8>()(static_cast<u8>(ah.type));
    }
};
