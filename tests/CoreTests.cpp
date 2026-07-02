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

class BinaryAsset : public AssetT<Vector<u8>, AssetType::Binary> {
public:

};


AssetLoadResult TestLoadBinaryAsset(const String &path) {

    AssetLoadResult res = {};
    auto data = Utils::LoadBinaryFromPath(path);

    return {};
}

void TestUnloadBinaryAsset(Asset *a) {

}


TestResult AssetManager_CanLoadAsset(AppState *e) {

    e->m_AssetManager.ProvideAssetFactory(
        AssetType::Binary,
        TestLoadBinaryAsset,
        TestUnloadBinaryAsset

    );
    return TestResult::Pass();
}

TEST_APP_BEGIN_SUITE("Core Tests", MEGABYTES(128))

ADD_TEST(ExampleTest)
ADD_TEST(ExampleTest2)
ADD_TEST(ExampleTest3)

ADD_TEST(AssetManager_CanLoadAsset)

TEST_APP_END_SUITE()