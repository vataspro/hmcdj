#include <hmcdj/utils/ensemble.h>

#include <regex>

/*
    * isValidStartingType

    Checks that a string is a valid Grid starting type,
    with the valid starting types being:
    - HotStart
    - TepidStart
    - ColdStart
 */
bool isValidStartingType(const std::string& startingType) {
  return startingType == "HotStart" || startingType == "TepidStart" ||
         startingType == "ColdStart";
}

/* Identify the base directory for all hmcdj output */
static std::filesystem::path getBaseDir() {
  if (const char* chosenDir = std::getenv("HMCDJ_BASE_PATH")) {
    return std::filesystem::path(chosenDir);
  }
  if (const char* homeDir = std::getenv("HOME")) {
    return std::filesystem::path(homeDir) / "hmcdj_ensembles";
  }
  return std::filesystem::path(".");
}

/* Obtain the path for a given track, relative to the base path of the deck */
static std::filesystem::path getEnsembleDirectoryPath(EnsembleReader* ensemble,
                                                      djParameterList params) {
  std::filesystem::path path = std::format("Nc{}", Grid::Nc);
  for (auto& param : params) {
    path = path / param.get().toString();
  }
  path = path / ensemble->GetDimString() / ensemble->StartingType;
  return path;
}

/* Create the subdirectories needed for a given track's output directory */
static void createEnsembleDirectories(std::filesystem::path baseDir) {
  std::filesystem::create_directories(baseDir / "cnfg");
  std::filesystem::create_directory(baseDir / "rand");
  std::filesystem::create_directory(baseDir / "logs");
}

/* Create the directory for the specified ensemble, and return the path to it */
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

/*
 * Ensemble Reader Initialisation
    Read an HMCDJ yaml file.

    Load the Grid job parameters.

    Check that the requested contents exist and raise
    an error if an issue occurs.
 */
// test -- use track.yaml and verify that we get the right parameters
EnsembleReader::EnsembleReader(const std::string deckNm,
                               const std::string filename,
                               djParameterList params,
                               pathCallback ensembleDirectoryPathOverride)
    : Parameters(params), deckName(deckNm) {
  // Load the parameters from the yaml file
  try {
    // Load the yaml file
    YAML::Node track = YAML::LoadFile(filename);

    // Loop over the highest level namespaces
    // Ensure that only 'Global' and deckName are present
    for (const auto& pair : track) {
      std::string name = pair.first.as<std::string>();
      if ((name != "Global") && (name != deckName)) {
        std::cerr << "Error: " << name << " is an invalid namespace"
                  << " for deck " << deckName << std::endl;

        exit(EXIT_FAILURE);
      }
    }

    YAML::Node global = track["Global"];

    /* VOLUME */
    // Grid lattice parameters
    nt = global["volume"]["nt"].as<int>();
    nx = global["volume"]["nx"].as<int>();
    ny = global["volume"]["ny"].as<int>();
    nz = global["volume"]["nz"].as<int>();

    /* CHECKPOINTING */
    saveInterval = global["checkpoint"]["saveInterval"].as<int>();
    format = global["checkpoint"]["format"].as<std::string>();

    /* HMC Parameters */
    trajL = global["HMC"]["MD"]["trajL"].as<double>();
    MDsteps = global["HMC"]["MD"]["MDsteps"].as<int>();

    Thermalisations = global["HMC"]["Thermalisations"].as<int>();
    Trajectories = global["HMC"]["Trajectories"].as<int>();
    StartingType = global["HMC"]["StartingType"].as<std::string>();

    /* Acceptance rate tuning parameters */
    if (global["AcceptanceRateTuning"]) {  // Acceptance rate tuning is
                                           // activated if the
                                           // Global::AcceptanceRateTuning
      AcceptanceTuningActive = true;       // namespace is defined in the track

      num_tuning_samples =
          global["AcceptanceRateTuning"]["num_tuning_samples"].as<int>();
      total_num_init_skips =
          global["AcceptanceRateTuning"]["total_num_init_skips"].as<int>();
      target_rate = global["AcceptanceRateTuning"]["target_rate"].as<double>();
      target_rate_tol =
          global["AcceptanceRateTuning"]["target_rate_tol"].as<double>();
      monitor_every = global["AcceptanceRateTuning"]["monitor_every"].as<int>();
      max_tuning_steps =
          global["AcceptanceRateTuning"]["max_tuning_steps"].as<int>();

      /* Check acc rate tuning inputs */
      if (Thermalisations >= total_num_init_skips) {
        std::cerr << "Acceptance rate tuning parameter 'total_num_init_skips' "
                     "should be greater than the number of thermalisations"
                  << std::endl;
        std::exit(EXIT_FAILURE);
      }

    } else {
      AcceptanceTuningActive = false;
    }

    /* Checks */
    /* Check that the Starting Type is valid */
    if (not isValidStartingType(StartingType)) {
      std::cerr << "Please provide a valid starting type, 'HotStart',"
                   "'ColdStart' or 'TepidStart'"
                << std::endl;
      std::exit(EXIT_FAILURE);
    }

    /* Read other HMCDJ parameters specific to the deck */
    if (!Parameters.empty()) {
      getParams(track);
    }

    /* Pathing: Ensemble directory depends on having read deck-specific
     * parameters */
    EnsembleDirectory = getEnsembleDirectory(this, deckNm, params,
                                             ensembleDirectoryPathOverride);
    config_prefix = (EnsembleDirectory / "cnfg" / "ckpoint_lat").string();
    rng_prefix = (EnsembleDirectory / "rand" / "ckpoint_rng").string();

    /* Dynamic Start: Depends on knowing ensemble directory */
    setStart(Thermalisations);  // Choose the StartingType and
                                // StartingTrajectory dynamically by checking
                                // the enseble home directory for configurations

  } catch (const YAML::Exception& e) {  // protect against mistake in yaml file
    std::cerr << "Error loading yaml file: " << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}

/*
 * Grid Dimensions String
    Returns a std::string representing the lattice 4-volume in the format Grid
 expects.
*/
const std::string EnsembleReader::GetDimString() {
  return std::format("{}.{}.{}.{}", nx, ny, nz, nt);
}

/*
 * Grid Dimensions String
    Returns the lattice 4-volume as a C string such that Grid will read it.
*/
const char* EnsembleReader::GetDimStringPointer() {
  dimString = GetDimString();
  return dimString.c_str();
}

/*
 * getParams
     Reads other parameters the user provides to the deck in the
     track["HMCDJ"]["Parameters"] section of the track (YAML file).

     These are all assumed to be double (floating point) parameters.
 */
void EnsembleReader::getParams(const YAML::Node track) {
  try {
    const YAML::Node& params = track[deckName];
    // Loop over the parameters and load them
    for (auto& param : Parameters) {
      param.get().readFromYAML(params);
    }

  } catch (const YAML::Exception& e) {  // protect against mistake in yaml file
    std::cerr << "Error loading yaml file: " << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}

/*
 * setStart
    Chooses the correct Starting Type and Starting Trajectory for
    the run.

    This is done by checking in the Ensemble Home Directory for files
    matching "config_prefix".
      If it does exist,  set the Starting Type
    to Checkpoint Start and  the largest configuration as the Starting
    Trajetory.
      Else, use the chosen defaults from the track.

 */
void EnsembleReader::setStart(int targetThermalisations) {
  // Initialise the starting trajectory to 0
  // update if configurations present
  StartingTrajectory = 0;

  // This regular expression matches the grid output configuration file names
  std::string config_prefix_basename =
      std::filesystem::path(config_prefix).filename();
  std::regex pattern("^" + config_prefix_basename + R"(\.(\d+)$)");

  /* Loop over the ensemble directory contents
      match any configuration files
      and find last configuration
  */
  for (const auto& entry :
       std::filesystem::directory_iterator(EnsembleDirectory / "cnfg")) {
    if (!entry.is_regular_file()) {
      continue;
    }  // check that entry is a file

    const std::string filename = entry.path().filename().string();
    std::smatch match;

    // try to match the regex to the file name and get the number
    if (std::regex_match(filename, match, pattern)) {
      int configNumber = std::stoi(match[1].str());
      StartingTrajectory = std::max(configNumber, StartingTrajectory);
    }
  }

  /* If there are configurations present
      set the starting type to checkpoint start
      and the starting trajectory as the last trajectory
  */
  if (StartingTrajectory > 0) {
    StartingType = "CheckpointStart";

    // Ensure that a run that died before finishing the initial thermalisation
    // stage gets the full requested thermalisation
    if (targetThermalisations > StartingTrajectory) {
      Thermalisations = targetThermalisations - StartingTrajectory;
    } else {
      Thermalisations = 0;
    }
  }
}
