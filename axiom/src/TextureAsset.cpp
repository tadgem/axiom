#include "Assets/TextureAsset.hpp"

#include "Core/Utils.hpp"

void OnProcessTextureAssetTransient(axm::AssetTransient* data)
{
	using namespace axm;
}

axm::TextureAssetFactory::TextureAssetFactory(vfspp::VirtualFileSystem& vfs, GPU& gpu)
    : AssetFactory(vfs, AssetType::Texture), m_GPU(gpu)
{}

axm::AssetLoadResult axm::TextureAssetFactory::LoadAsset(const Filesystem::path& path) const
{
	vfspp::IFilePtr file = m_FileSystem.OpenFile(path.generic_string(), vfspp::IFile::FileMode::Read);

	if (!file)
	{
		return AssetLoadResult::Fail("File could not be opened");
	}

	const u64    size = file->Size();
	DynArray<u8> data(size);

	file->Read(data);
	file->Close();

	auto cpuTexture = textures::LoadCPUTextureDataFromMemory(data.data(), data.size());

	if (!cpuTexture.m_Data)
	{
		return AssetLoadResult::Fail("Failed to decode texture data");
	}

	AssetLoadResult result       = {};

	auto*           textureAsset = AXM_NEW(TextureAsset, String(path.string()), {});
	auto*           transient    = AXM_NEW(TextureAssetTransient, textureAsset, std::move(cpuTexture));

	// TODO: Can we enforce this as a compile error?
	// SPECIFICALLY, we need to downcast a AssetTransient<T> to a AssetTransient*
	// so that the code that inspects on the variant (m_Next) does not fall apart.

	transient->m_NumSteps        = 1;
	result.m_Next                = dynamic_cast<AssetTransient*>(transient);

	return result;
}

void axm::TextureAssetFactory::UnloadAsset(Asset* asset) const
{
	auto* tex = dynamic_cast<TextureAsset*>(asset);
	if (tex == nullptr)
	{
		return;
	}

	if (tex->m_Data.m_Valid)
	{
		tex->m_Data.Free(m_GPU.m_VK);
		tex->m_Data = Texture::BAD();
	}
}

void axm::TextureAssetFactory::ProcessAssetTransient(AssetTransient* data) const
{
	auto*         transient = dynamic_cast<TextureAssetTransient*>(data);
	TextureAsset* tex       = transient->GetConcreteAsset();

	tex->m_Data             = textures::CreateTexture2D(m_GPU,
                                            transient->m_TransientData.m_Data,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            transient->m_TransientData.m_Width,
                                            transient->m_TransientData.m_Height);
	transient->m_TransientData.Release();

	transient->m_CurrentStep++;
}

void axm::TextureAssetFactory::OnTransientComplete(AssetTransient* data) const {}
