#include "AxiomTestFramework.hpp"

using namespace axm;


TestResult AssetManager_CanProvideFactory(AppState* e) {

    using TestBinaryAsset = AssetT<u64, AssetType::Binary>;


    auto load = [](const String& path) { return AssetLoadResult { .m_Next = AXM_NEW(TestBinaryAsset, path, 2305) }; };
    auto unload = [](Asset* a) {
        auto* b = dynamic_cast<TestBinaryAsset*>(a);
        AXM_DELETE(b);
    };

    e->m_AssetManager.ProvideAssetFactory(AssetType::Binary, load, unload);
    e->m_AssetManager.LoadAsset("test_resources/test.bin", AssetType::Binary);

    while (e->m_AssetManager.AnyAssetsLoading()) {
        e->m_AssetManager.Update();
    }
    auto* binary = e->m_AssetManager.GetAsset<TestBinaryAsset>("test_resources/test.bin");

    AXM_TEST_ASSERT(binary->m_Data == 2305, "BinaryAsset asset factory not invoked!");

    return TestResult::Pass();
}

TestResult AssetManager_CanLoadAsset(AppState* e) {

    using TestBinaryAsset = AssetT<Vector<u8>, AssetType::Binary>;

    auto load = [](const String& path) {
        auto bin = Utils::LoadBinaryFromPath(path);
        return AssetLoadResult { .m_Next = AXM_NEW(TestBinaryAsset, path, std::move(bin)) };
    };
    auto unload = [](Asset* a) {
        auto* b = dynamic_cast<TestBinaryAsset*>(a);
        AXM_DELETE(b);
    };

    e->m_AssetManager.ProvideAssetFactory(AssetType::Binary, load, unload);
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

AXM_BEGIN_TESTS("Core Tests")

AXM_ADD_TEST(AssetManager_CanProvideFactory)
AXM_ADD_TEST(AssetManager_CanLoadAsset)

AXM_END_TESTS()
