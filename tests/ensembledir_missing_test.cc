#include "run_test_helpers.h"

/* Check that a directory is correctly created when no base directory or home
 * directory are specified */
/* Disabled as tests are run with `mpirun`, which depends on a user home
 * directory existing */
TEST(EnsembleDirectoryTest, DISABLED_TestEnsembleDirectoryWithNoDir) {
  std::string testName = "hmcdj_nodir";
  TemporaryDirectory tmpDir(testName, true);
  TemporaryEnvironmentSuppress baseDir("HMCDJ_BASE_PATH");
  TemporaryEnvironmentSuppress homeDir("HOME");
  DJRun testRun(testName, nullptr, false);

  EXPECT_TRUE(std::filesystem::exists(
      tmpDir.getDirectoryPath() / "hmcdj_ensembles" / "NoParams" /
      std::format("Nc{}", Grid::Nc) / "4.4.4.4" / "HotStart" / "cnfg"));
  EXPECT_TRUE(std::filesystem::exists(
      tmpDir.getDirectoryPath() / "hmcdj_ensembles" / "NoParams" /
      std::format("Nc{}", Grid::Nc) / "4.4.4.4" / "HotStart" / "rand"));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
