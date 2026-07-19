#pragma once

#include <Grid/Grid.h>
#include <hmcdj/utils/parameter.h>
#include <yaml-cpp/yaml.h>

#include <cstdlib>
#include <filesystem>

/* Ensemble Reader */
class EnsembleReader {
 private:
  /* Lattice dimensions */
  int nt, nx, ny, nz;     // Lattice dims
  std::string dimString;  // Lattice dimensions string

 public:
  /* Callback function that can be used by EnsembleReader to obtain a path */
  typedef std::filesystem::path (*pathCallback)(EnsembleReader*,
                                                djParameterList);
  /* Checkpointing */
  int saveInterval;
  std::string config_prefix, rng_prefix, format;
  /* HMC parameters */
  double trajL;
  int initialMDsteps, Thermalisations, Trajectories;
  std::string StartingType;
  /* Dynamic Trajectory Initialisation */
  int StartingTrajectory;
  /* HMCDJ metadata */
  std::filesystem::path EnsembleDirectory;  // The ensemble/chain home directory
  djParameterList Parameters;               // Vector of parameters,
                                            // individually wrapped
                                            // in the Base class.
  std::string deckName;

  /* Acceptance rate tuning parameters */
  int tuningCycleTrajectories;         // Number of trajectories per tuning step
  int rethermalisationTrajectories;    // Steps discarded after each change in
                                       // MDsteps
  double targetAcceptance;             // Target acceptance rate
  double deltaTargetAcceptance;        // Target acceptance rate tolerance
  double monitoringCycleTrajectories;  // After tuning check the acceptance rate
                                       // this frequently
  int maxTuningTrajectories;           //

  // Constructor - reads and loads the parameters from the yaml file
  EnsembleReader(const std::string deckName, const std::string filename,
                 djParameterList Parameters,
                 pathCallback ensembleDirectoryPathOverride);

  /* Methods */
  // Gets the dimension string
  const std::string GetDimString();

  // Gets a char pointer to the dimension string
  const char* GetDimStringPointer();

  // Reads the parameters in track["HMCDJ"]["Parameters"]
  void getParams(const YAML::Node track);

  // Chooses the correct starting type and starting trajectory
  void setStart(int targetThermalisations);
};

/* Expose pathCallback for ease of use in code needing to pass a callback */
typedef EnsembleReader::pathCallback pathCallback;

/* Checks that a string is a valid Grid starting type */
bool isValidStartingType(const std::string& startingType);

std::filesystem::path getEnsembleDirectory(
    EnsembleReader* ensemble, std::string deckName, djParameterList params,
    pathCallback ensembleDirectoryPathOverride);
