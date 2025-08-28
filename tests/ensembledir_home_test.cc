#include "run_test_helpers.h"

/* Check that a directory is correctly created when no base directory is
 * specified, but $HOME exists */
TEST(EnsembleDirectoryTest, TestEnsembleDirectoryWithHomeDir) {
  std::string testName = "hmcdj_homedir";
  TemporaryDirectory tmpDir(testName);

  // Ensure that we go down the home directory code path by suppressing any base
  // path from the environment
  TemporaryEnvironmentSuppress baseDir("HMCDJ_BASE_PATH");

  // Ensure that we don't litter the user's home directory by creating a
  // temporary fake home directory
  TemporaryEnvironmentOverride homeDir("HOME",
                                       tmpDir.getDirectoryPath().string());

  DJRun testRun(testName);

  // Target directories created
  EXPECT_TRUE(std::filesystem::exists(
      tmpDir.getDirectoryPath() / "hmcdj_ensembles" / "NoParams" /
      std::format("Nc{}", Grid::Nc) / "4.4.4.4" / "HotStart" / "cnfg"));
  EXPECT_TRUE(std::filesystem::exists(
      tmpDir.getDirectoryPath() / "hmcdj_ensembles" / "NoParams" /
      std::format("Nc{}", Grid::Nc) / "4.4.4.4" / "HotStart" / "rand"));

  // Checkpoints being put in correct directory is tested in
  // ensembledir_base_test.cc, so doesn't need to be checked again here
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
