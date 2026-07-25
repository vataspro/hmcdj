#include "run_test_helpers.h"

/* Check that a directory is correctly created when no base directory or home
 * directory are specified */
TEST(EnsembleDirectoryTest, TestEnsembleDirectoryWithNoDir) {
#if defined(GRID_COMMS_MPI) || defined(GRID_COMMS_MPI3) || \
    defined(GRID_COMMS_MPIT)
  GTEST_SKIP() << "MPI requires a home directory to exist, "
                  "so this test cannot continue. "
                  "Try running again without `mpirun`.";
#endif
  std::string testName = "hmcdj_nodir";
  std::filesystem::path initialPath = std::filesystem::current_path();
  std::cout << initialPath << std::endl;
  TemporaryDirectory tmpDir(testName, true);
  std::filesystem::copy_file(
      initialPath / std::string(TOP_SRCDIR) / "tests/NoParamsTestTrack.yaml",
      tmpDir.getDirectoryPath() / "track.yaml");

  // Suppress both sources of base paths
  TemporaryEnvironmentSuppress baseDir("HMCDJ_BASE_PATH");
  TemporaryEnvironmentSuppress homeDir("HOME");

  // Create a run, but don't start it
  DJRun testRun(testName, nullptr, false);

  // Target directories created
  EXPECT_TRUE(std::filesystem::exists(
      tmpDir.getDirectoryPath() / "hmcdj_ensembles" / "NoParams" /
      std::format("Nc{}", Grid::Nc) / "4.4.4.4" / "HotStart" / "cnfg"));
  EXPECT_TRUE(std::filesystem::exists(
      tmpDir.getDirectoryPath() / "hmcdj_ensembles" / "NoParams" /
      std::format("Nc{}", Grid::Nc) / "4.4.4.4" / "HotStart" / "rand"));

  // Checkpoints being put in correct directory is tested in
  // ensembledir_base_test.cc, so doesn't need to be checked again here
  // As such, we don't need to start the run at all here
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
