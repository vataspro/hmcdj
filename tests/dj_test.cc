#include <gtest/gtest.h>
#include <hmcdj/utils/dj.h>

const std::string defaultGrid = "4.4.4.4";

std::vector<std::string> testGetGridArgv(const std::vector<std::string> argv,
                                         const std::string grid) {
  int argc = argv.size();
  char* gridArgv[argc];
  for (int argIndex = 0; argIndex < argc; argIndex++) {
    gridArgv[argIndex] = (char*)argv[argIndex].c_str();
  }

  char** gridResult = getGridArgv(argc, gridArgv, grid.c_str());
  std::vector<std::string> result;
  bool endOfArrayFound = false;
  for (int argIndex = 0; argIndex < argc + 2; argIndex++) {
    if (gridResult[argIndex] == nullptr) {
      endOfArrayFound = true;
      break;
    }
    result.push_back(gridResult[argIndex]);
  }
  if (!endOfArrayFound) {
    throw std::runtime_error("Unterminated argument list encountered");
  }

  return result;
}

void testDeathGetGridArgv(const std::vector<std::string> argv,
                          const std::string grid,
                          const std::string rejectedToken) {
  std::ostringstream message;
  message << "The argument " << rejectedToken << " was specified";
  EXPECT_DEATH(testGetGridArgv(argv, grid), message.str());
}

TEST(ArgvFilteringTest, CheckForbiddenTokensRejected) {
  /* Ensure that tokens that are controlled by the hmcdj input file are rejected
   * as command line arguments. */
  const std::vector<std::pair<std::vector<std::string>, std::string>> testData =
      {{{"hmcdj", "track.yaml", "--grid", "4.4.4.4"}, "--grid"},
       {{"hmcdj", "track.yaml", "--StartingType", "HotStart"},
        "--StartingType"},
       {{"hmcdj", "track.yaml", "--StartingTrajectory", "10"},
        "--StartingTrajectory"},
       {{"hmcdj", "track.yaml", "--Thermalizations", "10"},
        "--Thermalizations"},
       {{"hmcdj", "track.yaml", "--ParameterFile", "test.xml"},
        "--ParameterFile"},
       {{"hmcdj", "track.yaml", "--Trajectories", "100"}, "--Trajectories"}};
  for (auto const& [hmcdjArgv, rejectedToken] : testData) {
    testDeathGetGridArgv(hmcdjArgv, defaultGrid, rejectedToken);
  }
}

TEST(ArgvFilteringTest, CheckForbiddenTokensRejectedAfterAllowed) {
  /* Ensure that disallowed tokens are rejected even after an allowed argument.
   */
  const std::vector<std::string> hmcdjArgv = {
      "hmcdj", "track.yaml", "--mpi", "1.1.1.1", "--Trajectories", "100"};
  testDeathGetGridArgv(hmcdjArgv, defaultGrid, "--Trajectories");
}

TEST(ArgvFilteringTest, CheckForbiddenTokensRejectedInInvalidGridLine) {
  /* Ensure that disallowed tokens are rejected even if they do not form valid
   * options to Grid. */
  const std::vector<std::string> hmcdjArgv = {"hmcdj", "track.yaml",
                                              "--Trajectories", "--grid"};
  testDeathGetGridArgv(hmcdjArgv, defaultGrid, "--Trajectories");
}

TEST(ArgFilteringTest, CheckMinimalCommandLine) {
  /* Ensure that a minimal hmcdj command line is correctly transformed. */
  const std::vector<std::string> hmcdjArgv = {"hmcdj", "track.yaml"};
  const std::vector<std::string> expectedArgv = {"hmcdj", "--grid", "4.4.4.4"};
  auto updatedArgv = testGetGridArgv(hmcdjArgv, defaultGrid);
  EXPECT_EQ(updatedArgv, expectedArgv);
}

TEST(ArgFilteringTest, CheckCommandLineWithMPI) {
  /* Ensure that an hmcdj command line including a Grid option is correctly
   * transformed. */
  const std::vector<std::string> hmcdjArgv = {"hmcdj", "track.yaml", "--mpi",
                                              "1.1.1.1"};
  const std::vector<std::string> expectedArgv = {"hmcdj", "--mpi", "1.1.1.1",
                                                 "--grid", "4.4.4.4"};
  auto updatedArgv = testGetGridArgv(hmcdjArgv, defaultGrid);
  EXPECT_EQ(updatedArgv, expectedArgv);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
