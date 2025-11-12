#include <gtest/gtest.h>
#include <hmcdj/utils/dj.h>
#include <stdlib.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

/* Helpers for timing tests.

   This header is used by all timing tests,
   and allows testing a full HMC run.
   Each HMC test run must be placed in a separate .cc file,
   since each will call `Grid_Init()`,
   and calling this multiple times in a single run
   will cause Grid to error out.

   Also contains helpers for setting and overriding
   system environment variables. */

const int DJSuccessfulExit = 128;

/* Create a temporary directory with a specified base name.
   Delete it again when the instance is destroyed or goes out of scope. */
class TemporaryDirectory {
 private:
  std::filesystem::path directoryPath;

 public:
  TemporaryDirectory(const std::string name) {
    const std::filesystem::path baseTempDir =
        std::filesystem::temp_directory_path();
    const std::filesystem::path nameTemplatePath =
        baseTempDir / (name + "XXXXXX");
    const size_t bufferSize = nameTemplatePath.string().length() + 1;
    char nameTemplate[bufferSize];
    strncpy(nameTemplate, nameTemplatePath.c_str(), bufferSize);

    char* tmpDirResult;
    tmpDirResult = mkdtemp(nameTemplate);
    directoryPath = std::string(tmpDirResult);
  }

  ~TemporaryDirectory() { std::filesystem::remove_all(directoryPath); }

  std::filesystem::path getDirectoryPath() { return directoryPath; }
};

/* Find the most recent-modified file in a directory
   matching the given pattern. */
std::filesystem::path mostRecentDirectory(const std::string prefix) {
  const std::filesystem::path baseTempDir =
      std::filesystem::temp_directory_path();
  std::filesystem::path mostRecentMatch;
  bool haveMatch = false;
  for (auto const dirEntry : std::filesystem::directory_iterator{baseTempDir}) {
    if (dirEntry.path().filename().string().rfind(prefix, 0) == 0) {
      if (haveMatch) {
        if (std::filesystem::last_write_time(dirEntry.path()) >
            std::filesystem::last_write_time(mostRecentMatch)) {
          mostRecentMatch = dirEntry.path();
        }
      } else {
        haveMatch = true;
        mostRecentMatch = dirEntry.path();
      }
    }
  }
  if (!haveMatch) {
    throw "No matches found";
  }
  return mostRecentMatch;
}

/* Represents a single test run of hmcdj. */
class DJRun {
 private:
  typedef Grid::GenericSpHMCRunner<Grid::MinimumNorm2> HMCWrapper;
  std::unique_ptr<DJ<HMCWrapper, DJSuccessfulExit> > hmcdj;
  std::unique_ptr<TemporaryDirectory> tmpDir;

 public:
  DJRun(const std::string name = "hmcdj") {
    tmpDir = std::unique_ptr<TemporaryDirectory>(new TemporaryDirectory(name));
    initialiseRun();
  }

  ~DJRun() {}

  /* Create a basic, no-parameter HMCDJ deck.
     Borrowed from decks/NoParams.cc */
  void initialiseRun() {
    const static double beta = 7.2;
    int argc = 2;
    std::string testTrackName =
        std::string(TOP_SRCDIR) + "/example_tracks/NoParamsTrack.yaml";
    const char* argv[] = {"hmcdj_test", testTrackName.c_str()};
    hmcdj = std::unique_ptr<DJ<HMCWrapper, DJSuccessfulExit> >(
        new DJ<HMCWrapper, DJSuccessfulExit>("NoParams", argc, (char**)argv));
    static Grid::SpWilsonGaugeActionR Waction(beta);
    static Grid::ActionLevel<HMCWrapper::Field> Level1(1);
    Level1.push_back(&Waction);
    hmcdj->TheHMC.TheAction.push_back(Level1);
  };

  void play() {
    std::filesystem::path initialPath = std::filesystem::current_path();
    std::filesystem::current_path(tmpDir->getDirectoryPath());

    hmcdj->Play();

    std::filesystem::current_path(initialPath);
  };

  std::filesystem::path getDirectoryPath() {
    return tmpDir->getDirectoryPath();
  };
};

/* Override a single environment variable.
   Reset to the original value when
   the instance is destroyed or goes out of scope. */
class TemporaryEnvironmentOverride {
 private:
  bool varAlreadyExists = false;
  std::string existingValue;
  std::string varName;

 public:
  TemporaryEnvironmentOverride(std::string name, std::string value) {
    varName = name;
    if (std::getenv(name.c_str()) != nullptr) {
      varAlreadyExists = true;
      existingValue = std::getenv(name.c_str());
    }
    setenv(name.c_str(), value.c_str(), true);
  };

  ~TemporaryEnvironmentOverride() {
    if (varAlreadyExists) {
      setenv(varName.c_str(), existingValue.c_str(), true);
    } else {
      unsetenv(varName.c_str());
    }
  };
};

/* Remove a single environment variable from the environment.
   Reset to the original value when
   the instance is destroyed or goes out of scope. */
class TemporaryEnvironmentSuppress : public TemporaryEnvironmentOverride {
 public:
  TemporaryEnvironmentSuppress(std::string name)
      : TemporaryEnvironmentOverride(name, "") {
    unsetenv(name.c_str());
  };
};
