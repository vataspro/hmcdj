#include "run_test_helpers.h"

/* Set a deadline in the past, such that hmcdj should quit after a single
 * trajectory. */
TEST(TimerDeathTest, TestAbortIfInsufficientTime) {
  const std::string testName = "hmcdj_short_deadline";
  // Pretend that Slurm has set job end time to January 1970
  TemporaryEnvironmentOverride slurmJobEndTime("SLURM_JOB_END_TIME", "0");

  DJRun testRun(testName);
  EXPECT_EXIT(testRun.play(), testing::ExitedWithCode(DJSuccessfulExit), "");

  const std::filesystem::path outputPath =
      getSubDir(mostRecentDirectory(testName)) / "cnfg";

  // Shouldn't have time to do a second trajectory, so expect to save after
  // first, and not see a second.
  EXPECT_TRUE(std::filesystem::exists(outputPath / "ckpoint_lat.1"));
  EXPECT_FALSE(std::filesystem::exists(outputPath / "ckpoint_lat.2"));

  // Check that we abort correctly in addition to writing a configuration.
  // If we save correctly but fail to abort, then we'll get a configuration
  // after saveInterval = 5.
  EXPECT_FALSE(std::filesystem::exists(outputPath / "ckpoint_lat.5"));

  // Need to clean up by ourselves as the subprocess containing the
  // TemporaryDirectory instance aborted
  std::filesystem::remove_all(mostRecentDirectory(testName));
  std::filesystem::remove_all(testRun.getDirectoryPath());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  return RUN_ALL_TESTS();
}
