#include <gtest/gtest.h>
#include <hmcdj/utils/logging.h>

#include "run_test_helpers.h"

// Check that teestdout writes what we expect to both expected destinations
TEST(LoggingTest, TeeStdoutTest) {
  TemporaryDirectory tmpDir("teestdout");
  const std::string outputFilename =
      (tmpDir.getDirectoryPath() / "test.out").string();

  std::string capturedStdout;
  {
    testing::internal::CaptureStdout();
    std::cout << "This should only appear on stdout." << std::endl;
    teestdout tee;
    std::cout << "This should be buffered, and appear both in stdout and in "
                 "the output file."
              << std::endl;
    tee.setOutput(outputFilename);
    std::cout
        << "This should output directly to both stdout and the output file."
        << std::endl;
    capturedStdout = testing::internal::GetCapturedStdout();
  }

  EXPECT_EQ(capturedStdout,
            "This should only appear on stdout.\nThis should be buffered, and "
            "appear both in stdout and in the output file.\nThis should output "
            "directly to both stdout and the output file.\n");

  std::ifstream rereadFile;
  rereadFile.open(outputFilename);
  std::ostringstream rereadStream;
  rereadStream << rereadFile.rdbuf();
  EXPECT_EQ(rereadStream.str(),
            "This should be buffered, and appear both in stdout and in the "
            "output file.\nThis should output directly to both stdout and the "
            "output file.\n");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
