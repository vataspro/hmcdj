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

  // Create a run, but don't start it
  DJRun testRun(testName, nullptr, false);

  // Target directories created
  EXPECT_TRUE(std::filesystem::exists(
      getSubDir(tmpDir.getDirectoryPath() / "hmcdj_ensembles") / "cnfg"));
  EXPECT_TRUE(std::filesystem::exists(
      getSubDir(tmpDir.getDirectoryPath() / "hmcdj_ensembles") / "rand"));

  // Checkpoints being put in correct directory is tested in
  // ensembledir_base_test.cc, so doesn't need to be checked again here
  // As such, we don't need to start the run at all here
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
