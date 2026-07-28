#include <gmock/gmock.h>

#include "run_test_helpers.h"

/* Ensure that a run terminates early
   if after run start it becomes apparent that tuning can't complete in time. */
TEST(AcceptanceDeathTest, TestAbortAfterStart) {
  const std::string testName = "hmcdj_short_acceptance";

  // Add a temporary home directory in case the test misbehaves, to not trample
  // the user home directory
  const std::filesystem::path initialPath = std::filesystem::current_path();
  TemporaryDirectory tmpHomeDir(testName, true);
  std::filesystem::copy_file(initialPath / std::string(TOP_SRCDIR) /
                                 "tests/AcceptanceShortTestTrack.yaml",
                             tmpHomeDir.getDirectoryPath() / "track.yaml");
  TemporaryEnvironmentOverride homeDir("HOME",
                                       tmpHomeDir.getDirectoryPath().string());
  DJRun testRun(testName);
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
