#include "AxiomTestFramework.hpp"

using namespace axm;

using TestBinaryAsset = AssetT<Vector<u8>, AssetType::Binary>;


class TestAssetFactory : public AssetFactory
{
public:
    TestAssetFactory() : AssetFactory(AssetType::Binary) { }

    NO_DISCARD AssetLoadResult LoadAsset(const String& path) const override {
        return AssetLoadResult { .m_Next = AXM_NEW(TestBinaryAsset, path, std::move(Utils::LoadBinaryFromPath(path))) };
    }
    void UnloadAsset(Asset* asset) const override {
        auto* b = dynamic_cast<TestBinaryAsset*>(asset);
        AXM_DELETE(b);
    }
    void ProcessAssetTransient(AssetTransient* data) const override { }
    ~TestAssetFactory() override = default;
};

TestResult AssetManager_CanProvideFactory(AppState* e) {


    TestAssetFactory* factory = e->m_AssetManager.AddAssetFactory<AssetType::Binary, TestAssetFactory>();
    e->m_AssetManager.LoadAsset("test_resources/test.bin", AssetType::Binary);

    AXM_TEST_ASSERT(factory, "BinaryAsset asset factory not invoked!");

    return TestResult::Pass();
}

TestResult AssetManager_CanLoadAsset(AppState* e) {
    e->m_AssetManager.AddAssetFactory<AssetType::Binary, TestAssetFactory>();
    e->m_AssetManager.LoadAsset("test_resources/test.bin", AssetType::Binary);

    while (e->m_AssetManager.AnyAssetsLoading()) {
        e->m_AssetManager.Update();
    }

    auto* binary = e->m_AssetManager.GetAsset<TestBinaryAsset>("test_resources/test.bin");

    AXM_TEST_ASSERT(binary->m_Data.size() == 16, "Binary Asset not expected size");
    AXM_TEST_ASSERT(binary->m_Data[0] == '0', "Expected first binary blob to be 0");
    AXM_TEST_ASSERT(binary->m_Data[15] == '1', "Expected last binary blob to be 1");


    return TestResult::Pass();
}

TestResult AssetManager_CanUnloadAsset(AppState* e) {
    e->m_AssetManager.AddAssetFactory<AssetType::Binary, TestAssetFactory>();

    e->m_AssetManager.LoadAsset("test_resources/test.bin", AssetType::Binary);

    while (e->m_AssetManager.AnyAssetsLoading()) {
        e->m_AssetManager.Update();
    }

    auto*       binary = e->m_AssetManager.GetAsset<TestBinaryAsset>("test_resources/test.bin");
    AssetHandle handle = binary->m_Handle;

    AXM_TEST_ASSERT(binary, "Binary Asset not valid");

    e->m_AssetManager.UnloadAsset(handle);

    while (e->m_AssetManager.AnyAssetsUnloading()) {
        e->m_AssetManager.Update();
    }

    binary = e->m_AssetManager.GetAsset<TestBinaryAsset>(handle);

    AXM_TEST_ASSERT(!binary, "Binary asset should not be valid, it has been unloaded")

    return TestResult::Pass();
}

TestResult AssetManager_CanProcessTransient(AppState* e) {
    e->m_AssetManager.AddAssetFactory<AssetType::Texture, TextureAssetFactory>(e->m_Device);

    e->m_AssetManager.LoadAsset("test_resources/checkerboard.jpg", AssetType::Texture);

    AXM_TEST_ASSERT(e->m_AssetManager.AnyAssetsLoading(), "Texture load should have been enqueued");

    int count = 0;

    while (e->m_AssetManager.AnyAssetsLoading()) {
        e->m_AssetManager.Update();
        count++;
    }

    TextureAsset* texture = e->m_AssetManager.GetAsset<TextureAsset>("test_resources/checkerboard.jpg");

    AXM_TEST_ASSERT(texture, "Texture should be loaded after asset manager signals no more loading ops.")
    AXM_TEST_ASSERT(texture->m_Data.m_GPUTexture != nullptr, "a GPU Texture should have been allocated for this load");

    return TestResult::Pass();
}

TestResult AssetManager_NoFactoryForAssetType(AppState* e) {

    e->m_AssetManager.LoadAsset("test_resources/checkerboard.jpg", AssetType::Texture);

    AXM_TEST_ASSERT(!e->m_AssetManager.AnyAssetsLoading(), "Texture load should not have been enqueued, no factory");

    return TestResult::Pass();
}


TestResult AssetManager_NonExistantAssetNotEnqueued(AppState* e) {
    e->m_AssetManager.AddAssetFactory<AssetType::Texture, TextureAssetFactory>(e->m_Device);
    e->m_AssetManager.LoadAsset("test_resources/wrong.jpg", AssetType::Texture);

    AXM_TEST_ASSERT(!e->m_AssetManager.AnyAssetsLoading(), "Texture load should not have been enqueued, file does not exist");

    return TestResult::Pass();
}

AXM_BEGIN_TESTS("Core Tests")

AXM_ADD_TEST(AssetManager_CanProvideFactory)
AXM_ADD_TEST(AssetManager_CanLoadAsset)
AXM_ADD_TEST(AssetManager_CanUnloadAsset)
AXM_ADD_TEST(AssetManager_CanProcessTransient)
AXM_ADD_TEST(AssetManager_NoFactoryForAssetType)
AXM_ADD_TEST(AssetManager_NonExistantAssetNotEnqueued)

AXM_END_TESTS()
