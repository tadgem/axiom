#include "Assets/Defaults.hpp"

#include "Assets/Model.hpp"
#include "Assets/TextureAsset.hpp"
#include "Core/Engine.hpp"

void axm::AddDefaultAssetHandlers(AxiomEngine& e)
{
	e.m_AssetManager.AddAssetFactory<AssetType::Texture, TextureAssetFactory>(*e.m_GPU);
	e.m_AssetManager.AddAssetFactory<AssetType::Model, ModelAssetFactory>(e.m_GPU->m_VK);
}
