#include <gmock/gmock.h>

#include "run_test_helpers.h"

/* Check that a directory is correctly created when the base directory is
 * specified */
TEST(EnsembleDirectoryTest, TestEnsembleDirectoryWithSpecifiedBaseDir) {
  std::string testName = "hmcdj_basedir";

  // Add a temporary home directory in case the test misbehaves, to not trample
  // the user home directory
  TemporaryDirectory tmpHomeDir(testName);
  TemporaryEnvironmentOverride homeDir("HOME",
                                       tmpHomeDir.getDirectoryPath().string());
  DJRun testRun(testName);

  std::filesystem::path ensemblePath = getSubDir(testRun.getDirectoryPath());

  // Target directories created
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "cnfg"));
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "rand"));
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "logs"));

  testRun.play();

  // Checkpoints put in correct directories
  EXPECT_FALSE(std::filesystem::exists(testRun.getDirectoryPath() / "cnfg" /
                                       "ckpoint_lat.5"));
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "cnfg" / "ckpoint_lat.5"));
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "rand" / "ckpoint_rng.5"));

  // Check logs are created correctly
  std::vector<std::string> logs = getLogs(ensemblePath);

  // We don't see the "Grid Finalize" block in this test
  // as this won't get printed until testRun goes out of scope,
  // but at that point the temporary directory holding the logs will be
  // deleted. As a result, we expect to see the footer of the configuration
  // save block. Standard decks will see a Grid Finalize block.
  for (auto log : logs) {
    EXPECT_THAT(log.c_str(),
                ::testing::EndsWith(" : Run is complete; finishing up.\n"));
  }
  EXPECT_EQ(logs.size(), 1);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
