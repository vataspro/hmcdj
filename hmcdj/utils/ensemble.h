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
  int MDsteps, Thermalisations, Trajectories;
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
  bool AcceptanceTuningActive;  // Flag signaling whther acceptance rate tuning
                                // is active
  int num_tuning_samples;       // Number of trajectories per tuning step
  int total_num_init_skips;     // NoMetropolisUntil + "thermalisation" steps
  double target_rate;           // Target acceptance rate
  double target_rate_tol;       // Target acceptance rate tolerance
  double
      monitor_every;  // After tuning check the acceptance rate this frequently

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
