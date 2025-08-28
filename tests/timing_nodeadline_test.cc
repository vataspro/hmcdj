#include "run_test_helpers.h"

/* Remove any Slurm deadline, and check that a run completes */
TEST(TimerTest, TestCompleteIfNoDeadline) {
  TemporaryEnvironmentSuppress slurmJobEndTime("SLURM_JOB_END_TIME");

  DJRun testRun("hmcdj_no_deadline");
  testRun.play();
  // saveInterval is 5, so don't expect 1 or 2
  std::filesystem::path cnfgPath =
      getSubDir(testRun.getDirectoryPath()) / "cnfg";
  EXPECT_FALSE(std::filesystem::exists(cnfgPath / "ckpoint_lat.1"));
  EXPECT_FALSE(std::filesystem::exists(cnfgPath / "ckpoint_lat.2"));

  // Do expect multiples of saveInterval
  EXPECT_TRUE(std::filesystem::exists(cnfgPath / "ckpoint_lat.5"));
  EXPECT_TRUE(std::filesystem::exists(cnfgPath / "ckpoint_lat.10"));

  // Trajectories is 10, so don't expect anything beyond this
  EXPECT_FALSE(std::filesystem::exists(cnfgPath / "ckpoint_lat.15"));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  return RUN_ALL_TESTS();
}
