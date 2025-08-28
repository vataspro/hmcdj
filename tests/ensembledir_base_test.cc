#include "run_test_helpers.h"

/* Check that a directory is correctly created when the base directory is
 * specified */
TEST(EnsembleDirectoryTest, TestEnsembleDirectoryWithSpecifiedBaseDir) {
  std::string testName = "hmcdj_basedir";
  TemporaryDirectory tmpDir(testName);

  // Add a temporary home directory in case the test misbehaves, to not trample
  // the user home directory
  TemporaryDirectory tmpHomeDir(testName);
  TemporaryEnvironmentOverride baseDir("HMCDJ_BASE_PATH",
                                       tmpDir.getDirectoryPath().string());
  TemporaryEnvironmentOverride homeDir("HOME",
                                       tmpHomeDir.getDirectoryPath().string());
  DJRun testRun(testName);

  std::filesystem::path ensemblePath = tmpDir.getDirectoryPath() / "NoParams" /
                                       std::format("Nc{}", Grid::Nc) /
                                       "4.4.4.4" / "HotStart";

  // Target directories created
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "cnfg"));
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "rand"));

  testRun.play();

  // Checkpoints put in correct directories
  EXPECT_FALSE(
      std::filesystem::exists(testRun.getDirectoryPath() / "ckpoint_lat.5"));
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "cnfg" / "ckpoint_lat.5"));
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "rand" / "ckpoint_rng.5"));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
