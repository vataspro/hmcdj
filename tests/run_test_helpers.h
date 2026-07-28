#include <gtest/gtest.h>
#include <hmcdj/utils/dj.h>
#include <stdlib.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

/* Helpers for tests requiring a hmcdj run.

   This header is used by all timing and output directorytests,
   and allows testing a full HMC run.
   Each HMC test run must be placed in a separate .cc file,
   since each will call `Grid_init()`,
   and calling this multiple times in a single run
   will cause Grid to error out.

   Also contains helpers for setting and overriding
   system environment variables. */

const int DJSuccessfulExit = 128;

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

/* Create a temporary directory with a specified base name.
   Delete it again when the instance is destroyed or goes out of scope. */
class TemporaryDirectory {
 private:
  std::filesystem::path directoryPath;
  std::filesystem::path initialPath;
  bool resetDirectory;

 public:
  TemporaryDirectory(const std::string name, bool changeDirectory = false) {
    initialPath = std::filesystem::current_path();
    resetDirectory = changeDirectory;

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
    if (changeDirectory) {
      std::filesystem::current_path(directoryPath);
    }
  }

  ~TemporaryDirectory() {
    if (resetDirectory) {
      std::filesystem::current_path(initialPath);
    }
    std::filesystem::remove_all(directoryPath);
  }

  std::filesystem::path getDirectoryPath() const { return directoryPath; }
};

std::filesystem::path getSubDir(std::filesystem::path baseDir) {
  return baseDir / "NoParams" / std::format("Nc{}", Grid::Nc) / "4.4.4.4" /
         "HotStart";
}

class DJRun {
 private:
  typedef Grid::GenericSpHMCRunner<Grid::MinimumNorm2> HMCWrapper;
  std::shared_ptr<DJ<HMCWrapper, DJSuccessfulExit> > hmcdj;
  std::shared_ptr<TemporaryDirectory> tmpDir;
  bool setBasePath;

 public:
  DJRun(const std::string name = "hmcdj",
        pathCallback ensembleDirectoryPathOverride = nullptr,
        bool setBasePath = true)
      : setBasePath(setBasePath) {
    tmpDir = std::make_unique<TemporaryDirectory>(name);
    initialiseRun(ensembleDirectoryPathOverride);
  }

  ~DJRun() {}

  /* Create a basic, no-parameter HMCDJ deck.
     Borrowed from decks/NoParams.cc */
  void initialiseRun(pathCallback ensembleDirectoryPathOverride) {
    TemporaryEnvironmentOverride baseDir("HMCDJ_BASE_PATH",
                                         getDirectoryPath().string());
    if (!setBasePath) {
      // Use the underlying unsetenv function,
      // because we putting the override class into an if block would prevent it
      // working
      unsetenv("HMCDJ_BASE_PATH");
    }
    const static double beta = 7.2;
    const int argc = 2;
    std::string testTrackName = "track.yaml";
    if (!std::filesystem::exists(testTrackName)) {
      testTrackName = std::string(TOP_SRCDIR) + "/tests/NoParamsTestTrack.yaml";
    }
    const char* argv[] = {"hmcdj_test", testTrackName.c_str()};
    const bool useReducedStorage = true;
    hmcdj = std::make_shared<DJ<HMCWrapper, DJSuccessfulExit> >(
        "NoParams", argc, (char**)argv, useReducedStorage,
        ensembleDirectoryPathOverride);
    static Grid::SpWilsonGaugeActionR Waction(beta);
    static Grid::ActionLevel<HMCWrapper::Field> Level1(1);
    Level1.push_back(&Waction);
    hmcdj->TheHMC.TheAction.push_back(Level1);
  };

  void play() {
    TemporaryEnvironmentOverride baseDir("HMCDJ_BASE_PATH",
                                         getDirectoryPath().string());
    if (!setBasePath) {
      // Use the underlying unsetenv function,
      // because we putting the override class into an if block would prevent it
      // working
      unsetenv("HMCDJ_BASE_PATH");
    }
    const std::filesystem::path initialPath = std::filesystem::current_path();
    std::filesystem::current_path(tmpDir->getDirectoryPath());

    hmcdj->Play();

    std::filesystem::current_path(initialPath);
  };

  std::filesystem::path getDirectoryPath() const {
    return tmpDir->getDirectoryPath();
  };
  std::shared_ptr<DJ<HMCWrapper, DJSuccessfulExit> > getDJ() const {
    return hmcdj;
  };
};

std::filesystem::path mostRecentDirectory(const std::string prefix,
                                          const bool checkSubDir = true) {
  const std::filesystem::path baseTempDir =
      std::filesystem::temp_directory_path();
  std::filesystem::path mostRecentMatch;
  bool haveMatch = false;
  for (auto const dirEntry : std::filesystem::directory_iterator{baseTempDir}) {
    if (dirEntry.path().filename().string().rfind(prefix, 0) == 0 &&
        (!checkSubDir ||
         std::filesystem::exists(getSubDir(dirEntry.path()) / "logs"))) {
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

std::vector<std::string> getLogs(std::filesystem::path ensemblePath) {
  std::vector<std::string> result;
  for (auto& entry :
       std::filesystem::directory_iterator(ensemblePath / "logs")) {
    if (entry.is_regular_file()) {
      std::ifstream rereadFile;
      rereadFile.open(entry.path());
      std::ostringstream rereadStream;
      rereadStream << rereadFile.rdbuf();
      result.push_back(rereadStream.str());
    }
  }
  return result;
}
