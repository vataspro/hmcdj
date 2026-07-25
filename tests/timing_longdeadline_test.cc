#include "run_test_helpers.h"

/* Set a deadline far in the future, and check that a run completes */
TEST(TimerTest, TestCompleteWithLongDeadline) {
  // Get a time maximally far in the future
  // std::chrono supports 35-bit integers for constructing timespans in seconds,
  // so 1l << 33 (i.e. 2^33) gives the maximum time we can request
  // This corresponds to March 2242, so we're not in danger of the year 2038
  // problem
  TemporaryEnvironmentOverride slurmJobEndTime("SLURM_JOB_END_TIME",
                                               std::to_string(1l << 33));

  DJRun testRun("hmcdj_long_deadline");
  testRun.play();

  std::filesystem::path cnfgPath =
      getSubDir(testRun.getDirectoryPath()) / "cnfg";

  // saveInterval is 5, so don't expect 1 or 2
  EXPECT_FALSE(std::filesystem::exists(cnfgPath / "ckpoint_lat.1"));
  EXPECT_FALSE(std::filesystem::exists(cnfgPath / "ckpoint_lat.2"));

  // Do expect multiples of saveInterval
  EXPECT_TRUE(std::filesystem::exists(cnfgPath / "ckpoint_lat.5"));
  EXPECT_TRUE(std::filesystem::exists(cnfgPath / "ckpoint_lat.10"));

  // Trajectories is 20, so don't expect anything beyond this
  EXPECT_FALSE(std::filesystem::exists(cnfgPath / "ckpoint_lat.25"));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  return RUN_ALL_TESTS();
}
