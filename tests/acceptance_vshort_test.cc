#include <gmock/gmock.h>

#include "run_test_helpers.h"

/* Ensure that a run terminates early
   if it is clear at run start that it won't have sufficient time to tune. */
TEST(AcceptanceDeathTest, TestAbortWithoutStarting) {
  const std::string testName = "hmcdj_vshort_acceptance";

  // Add a temporary home directory in case the test misbehaves, to not trample
  // the user home directory
  TemporaryDirectory tmpHomeDir(testName);
  TemporaryEnvironmentOverride homeDir("HOME",
                                       tmpHomeDir.getDirectoryPath().string());
  DJRun testRun(testName);
  testRun.getDJ()->extraCPPars.acceptance->maxTuningTrajectories = 5;
  EXPECT_EXIT(testRun.play(), testing::ExitedWithCode(EXIT_FAILURE), "");

  const std::filesystem::path ensemblePath =
      getSubDir(mostRecentDirectory(testName));
  std::vector<std::string> logs = getLogs(ensemblePath);
  for (auto log : logs) {
    EXPECT_THAT(log.c_str(),
                ::testing::HasSubstr(" : Unable to tune acceptance in "
                                     "remaining trajectories; aborting.\n"));
  }

  // Need to clean up by ourselves as the subprocess containing the
  // TemporaryDirectory instance aborted
  // Since the failed test created both a temporary home directory,
  // and a temporary run directory,
  // we need to get the _two_ most recent directories and remove them
  std::filesystem::remove_all(tmpHomeDir.getDirectoryPath());
  std::filesystem::remove_all(mostRecentDirectory(testName));
  std::filesystem::remove_all(mostRecentDirectory(testName, false));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  return RUN_ALL_TESTS();
}
