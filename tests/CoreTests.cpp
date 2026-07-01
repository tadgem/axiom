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




TEST_APP_BEGIN_SUITE("Core Tests", MEGABYTES(256))

ADD_TEST(ExampleTest)
ADD_TEST(ExampleTest2)
ADD_TEST(ExampleTest3)

TEST_APP_END_SUITE()