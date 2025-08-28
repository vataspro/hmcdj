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
                                                      djParameterList params) {
  std::filesystem::path path = std::format("Nc{}", Grid::Nc);
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
    pathCallback ensembleDirectoryPathOverride) {
  std::filesystem::path ensembleDirectory = getBaseDir() / deckName;
  if (ensembleDirectoryPathOverride == nullptr) {
    ensembleDirectory =
        ensembleDirectory / getEnsembleDirectoryPath(ensemble, params);
  } else {
    ensembleDirectory =
        ensembleDirectory / ensembleDirectoryPathOverride(ensemble, params);
  }
  createEnsembleDirectories(ensembleDirectory);
  return ensembleDirectory;
}
