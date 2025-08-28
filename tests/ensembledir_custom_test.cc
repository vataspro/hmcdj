#include "run_test_helpers.h"

std::filesystem::path overridePath(EnsembleReader* ensemble,
                                   djParameterList params) {
  return std::filesystem::path("TestOverride");
}

/* Check that a directory is correctly created when a custom subdirectory is
 * specified */
TEST(EnsembleDirectoryTest, TestEnsembleDirectoryWithSpecifiedBaseDir) {
  std::string testName = "hmcdj_basedir";

  // Add a temporary home directory in case the test misbehaves, to not trample
  // the user home directory
  TemporaryDirectory tmpHomeDir(testName);
  TemporaryEnvironmentOverride homeDir("HOME",
                                       tmpHomeDir.getDirectoryPath().string());
  DJRun testRun(testName, overridePath);

  std::filesystem::path ensemblePath =
      testRun.getDirectoryPath() / "NoParams" / "TestOverride";

  // Target directories created
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "cnfg"));
  EXPECT_TRUE(std::filesystem::exists(ensemblePath / "rand"));

  // Checkpoints being put in correct directory is tested in
  // ensembledir_base_test.cc, so doesn't need to be checked again here
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
