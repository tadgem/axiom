#pragma once
#include "Prim.hpp"
#include "STL.hpp"

namespace axm {
    // TODO: Can we have a reflectable enum structure or macro helper?
    // e.g. something that lets us grab a human readable string name for each enum entry
    // need to be careful not to use for _all_ enums as string sizes would skyrocket.
    enum class AssetType : u8 { Unknown, Model, Texture, Shader, Audio, Text, Binary };

    /// <summary>
    /// Lightweight identifier for an arbitrary asset
    /// use in collections for fast look up
    /// </summary>
    struct AssetHandle {
        AssetType m_AssetType;
        str_hash m_PathHash;

        AssetHandle();
        AssetHandle(const String &p, const AssetType &type);

        bool operator==(const AssetHandle &o) const {
            return m_AssetType == o.m_AssetType && m_PathHash == o.m_PathHash;
        }

        bool operator<(const AssetHandle &o) const { return m_AssetType < o.m_AssetType && m_PathHash < o.m_PathHash; }
    };

    /// <summary>
    /// Allow asset to be loaded by asset manager system
    /// Giving concrete path and type of asset
    /// </summary>
    struct SerializableAssetHandle {
        AssetHandle m_Handle;
        String m_Path;

        SerializableAssetHandle(const String &p, const AssetType &type);
    };

    class Asset {
    public:
        Asset(const String &path, const AssetType &type);
        virtual ~Asset() = default;

        const String m_Path;
        const AssetHandle m_Handle;
    };

    template<typename AssetDataType, AssetType AssetTypeEnum>
    class AssetT : public Asset {
    public:
        AssetDataType m_Data;

        static constexpr AssetType kAssetEnumType = AssetTypeEnum;

        AssetT(const String &path, AssetDataType &&data) : Asset(path, AssetTypeEnum), m_Data(std::move(data)) {};

        ~AssetT() override = default;
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

        virtual ~AssetTransientData() = default;
    };

    template<typename AssetDataType, typename IntermediateDataType, AssetType AssetTypeEnum>
    class AssetTransientT : public AssetTransientData {
    public:
        IntermediateDataType m_TransientData;

        AssetTransientT(Asset *data, const IntermediateDataType &inter) :
            AssetTransientData(data), m_TransientData(inter) {}

        AssetT<AssetDataType, AssetTypeEnum> get_concrete_asset() {
            return static_cast<AssetT<AssetDataType, AssetTypeEnum>>(m_AssetDataPtr);
        }
    };

} // namespace axm

/// <summary>
/// Allow AssetHandle to be used as a key in unordered_map
/// </summary>
template<>
struct std::hash<axm::AssetHandle> {

    size_t operator()(const axm::AssetHandle &ah) const noexcept {
        return std::hash<str_hash>()(ah.m_PathHash) ^ std::hash<u8>()(static_cast<u8>(ah.m_AssetType));
    }
};
