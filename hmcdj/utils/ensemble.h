#include <Grid/Grid.h>
#include <hmcdj/utils/parameter.h>
#include <hmcdj/utils/utils.h>
#include <yaml-cpp/yaml.h>

#include <cstdlib>
#include <filesystem>

static std::filesystem::path getBaseDir() {
  if (const char* chosenDir = std::getenv("HMCDJ_BASE_PATH")) {
    return std::filesystem::path(chosenDir);
  }
  if (const char* homeDir = std::getenv("HOME")) {
    return std::filesystem::path(homeDir) / "hmcdj_ensembles";
  }
  return std::filesystem::path(".");
}

static std::filesystem::path getEnsembleDirectoryPath(EnsembleReader* ensemble,
                                                      std::string deckName,
                                                      djParameterList params) {
  std::filesystem::path path =
      getBaseDir() / deckName / std::format("Nc{}", Grid::Nc);
  for (auto& param : params) {
    path = path / param.get().toString();
  }
  path = path / ensemble->GetDimString() / ensemble->StartingType;
  return path;
}

static void createEnsembleDirectories(std::filesystem::path baseDir) {
  std::filesystem::create_directories(baseDir / "cnfg");
  std::filesystem::create_directory(baseDir / "rand");
}

std::filesystem::path getEnsembleDirectory(
    EnsembleReader* ensemble, std::string deckName, djParameterList params,
    std::filesystem::path (*ensembleDirectoryPathOverride)(EnsembleReader*,
                                                           std::string,
                                                           djParameterList)) {
  std::filesystem::path ensembleDirectory;
  if (ensembleDirectoryPathOverride == nullptr) {
    ensembleDirectory = getEnsembleDirectoryPath(ensemble, deckName, params);
  } else {
    ensembleDirectory =
        ensembleDirectoryPathOverride(ensemble, deckName, params);
  }
  createEnsembleDirectories(ensembleDirectory);
  return ensembleDirectory;
}
