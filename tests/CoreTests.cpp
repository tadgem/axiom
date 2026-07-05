#include "AxiomTestFramework.hpp"

using namespace axm;


TestResult ExampleTest(AppState *e) {

    return TestResult::Pass();
}

TestResult ExampleTest2(AppState *e) {

    return TestResult::Fail("Something bad happened");
}

TestResult ExampleTest3(AppState *e) {

    return TestResult::Pass();
}

using BinaryAsset =  AssetT<Vector<u8>, AssetType::Binary>;

static bool loaded = false;
AssetLoadResult TestLoadBinaryAsset(const String &path) {

    loaded = true;
    AssetLoadResult res = {};
    auto data = Utils::LoadBinaryFromPath(path);

    auto* binaryAsset = AXM_NEW(BinaryAsset, path, std::move(data));
    auto* asset = static_cast<Asset*>(binaryAsset);

    return  {
        .m_Next = asset,
        .m_NewAssetTasks = {},
        .m_SyncAssetCallbacks = {}
    };
}

void TestUnloadBinaryAsset(Asset *a) {
    loaded = false;
}


TestResult AssetManager_CanProvideFactory(AppState *e) {


    e->m_AssetManager.ProvideAssetFactory(
        AssetType::Binary,
        TestLoadBinaryAsset,
        TestUnloadBinaryAsset
    );

    e->m_AssetManager.LoadAsset("test_resources/test.bin", AssetType::Binary);

    while (e->m_AssetManager.AnyAssetsLoading()) {
        e->m_AssetManager.Update();
    }

    AXM_TEST_ASSERT(loaded, "BinaryAsset asset factory not invoked!");

    return TestResult::Pass();
}

AXM_BEGIN_TESTS("Core Tests", MEGABYTES(128))

AXM_ADD_TEST(ExampleTest)
AXM_ADD_TEST(ExampleTest2)
AXM_ADD_TEST(ExampleTest3)

AXM_ADD_TEST(AssetManager_CanProvideFactory)

AXM_END_TESTS()