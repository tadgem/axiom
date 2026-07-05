#include "AxiomTestFramework.hpp"

using namespace axm;


TestResult ExampleTest(AppState *e) { return TestResult::Pass(); }

TestResult ExampleTest2(AppState *e) { return TestResult::Fail("Something bad happened"); }

TestResult ExampleTest3(AppState *e) { return TestResult::Pass(); }


TestResult AssetManager_CanProvideFactory(AppState *e) {

    using TestBinaryAsset = AssetT<u64, AssetType::Binary>;


    auto load = [](const String &path) { return AssetLoadResult{.m_Next = AXM_NEW(TestBinaryAsset, path, 2305)}; };
    auto unload = [](Asset *a) {
        auto *b = dynamic_cast<TestBinaryAsset *>(a);
        AXM_DELETE(b);
    };

    e->m_AssetManager.ProvideAssetFactory(AssetType::Binary, load, unload);
    e->m_AssetManager.LoadAsset("test_resources/test.bin", AssetType::Binary);

    while (e->m_AssetManager.AnyAssetsLoading()) {
        e->m_AssetManager.Update();
    }
    auto *binary = e->m_AssetManager.GetAsset<TestBinaryAsset>("test_resources/test.bin");

    AXM_TEST_ASSERT(binary->m_Data == 2305, "BinaryAsset asset factory not invoked!");

    return TestResult::Pass();
}

AXM_BEGIN_TESTS("Core Tests")

AXM_ADD_TEST(ExampleTest)
AXM_ADD_TEST(ExampleTest2)
AXM_ADD_TEST(ExampleTest3)

AXM_ADD_TEST(AssetManager_CanProvideFactory)

AXM_END_TESTS()
