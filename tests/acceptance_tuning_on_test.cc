#include "run_test_helpers.h"

TEST(AcceptanceTuningTest, TestPlayBeforeTuneFails) {
  DJRun testRun("hmcdj_tune");
  EXPECT_EXIT(testRun.play(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  return RUN_ALL_TESTS();
}
