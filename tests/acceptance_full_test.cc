#include <gmock/gmock.h>

#include "run_test_helpers.h"

/* Ensure that a run terminates early
   if after run start it becomes apparent that tuning can't complete in time. */
TEST(AcceptanceDeathTest, TestAbortAfterStart) {
  const std::string testName = "hmcdj_full_acceptance";

  // Add a temporary home directory in case the test misbehaves, to not trample
  // the user home directory
  const std::filesystem::path initialPath = std::filesystem::current_path();
  TemporaryDirectory tmpHomeDir(testName, true);
  std::filesystem::copy_file(initialPath / std::string(TOP_SRCDIR) /
                                 "tests/AcceptanceFullTestTrack.yaml",
                             tmpHomeDir.getDirectoryPath() / "track.yaml");
  TemporaryEnvironmentOverride homeDir("HOME",
                                       tmpHomeDir.getDirectoryPath().string());
  DJRun testRun(testName);
  testRun.play();

  // Check output
  const std::filesystem::path ensemblePath =
      getSubDir(mostRecentDirectory(testName));
  std::vector<std::string> logs = getLogs(ensemblePath);
  for (auto log : logs) {
    EXPECT_THAT(log.c_str(),
                ::testing::HasSubstr(" : Run is complete; finishing up.\n"));
  }

  // Check acceptance
  std::vector<int> acceptHistory;
  Grid::XmlReader accReader((ensemblePath / "acceptance.xml").string());
  accReader.readDefault("acceptHistory", acceptHistory);
  int accepted = 0;
  const int count = 200;
  for (int trajectory = 1; trajectory <= count; trajectory++) {
    accepted += acceptHistory[acceptHistory.size() - trajectory];
  }
  const double meanAcceptance = accepted / 200.0;
  const double errorAcceptance = sqrt(meanAcceptance / count);

  EXPECT_LT(meanAcceptance - errorAcceptance, 0.89);
  EXPECT_GT(meanAcceptance + errorAcceptance, 0.81);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  return RUN_ALL_TESTS();
}
