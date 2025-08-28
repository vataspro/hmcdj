#include <hmcdj/utils/ensemble.h>
#include <hmcdj/utils/parameter.h>
#include <hmcdj/utils/utils.h>

/*
    * Guard
    Ensures that the program is called correctly.

    Further checks on the validity of the requested yaml file
    are implemented in the Ensemble Reader.
*/
void djGuard(int argc, char* argv[]) {
  // Usage
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " filename --<Other Grid arguments>"
              << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // Check that the file exists
  std::ifstream file(argv[1]);  // opens a filestream of argv[1]
  if (!file) {
    std::cerr << "File " << argv[1] << " does not exist!" << std::endl;
    if (static_cast<std::string>(argv[1]).substr(0, 2) == "--") {
      std::cerr << "The first provided argument should be the track"
                << std::endl;
    }
    std::exit(EXIT_FAILURE);
  }
}

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

/*
    * RNGManager Constructor

    The RNG Manager initialises an instance of the sitmo
    RNG engine from Grid's source code. This is deterministic
    and should be compiler indipendent.

    Using the Seed method, the RNG engine is seeded from the
    filename when the class is initialised.
 */
RNGManager::RNGManager(std::string filename) : engine() { Seed(filename); }

/* RNGManager Seeding */
void RNGManager::Seed(std::string filename) {
  engine.seed((md5FileToInt(filename)));
}

/* Generate Grid RNG string */
std::string RNGManager::GenerateGridRNGSeedString() {
  std::ostringstream RNGstr;
  std::uniform_int_distribution<int> dist(0, 100);

  RNGstr << dist(engine);

  for (int i = 1; i < 5; i++) {
    RNGstr << " ";
    RNGstr << dist(engine);
  }

  return RNGstr.str();
}

/* Get an integer hash from a file's contents */
uint32_t md5FileToInt(const std::string& filename) {
  constexpr std::size_t bufferSize = 4096;
  unsigned char buffer[bufferSize];
  unsigned char md5Digest[EVP_MAX_MD_SIZE];
  unsigned int md5Len = 0;

  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file");
  }

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) throw std::runtime_error("Failed to create EVP_MD_CTX");

  if (!EVP_DigestInit_ex(ctx, EVP_md5(), nullptr)) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("DigestInit failed");
  }

  while (file.read(reinterpret_cast<char*>(buffer), bufferSize) ||
         file.gcount() > 0) {
    if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
      EVP_MD_CTX_free(ctx);
      throw std::runtime_error("DigestUpdate failed");
    }
  }

  if (!EVP_DigestFinal_ex(ctx, md5Digest, &md5Len)) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("DigestFinal failed");
  }

  EVP_MD_CTX_free(ctx);

  return *reinterpret_cast<uint32_t*>(md5Digest);
}

/*
 * Ensemble Reader Initialisation
    Read an HMCDJ yaml file.

    Load the Grid job parameters.

    Check that the requested contents exist and raise
    an error if an issue occurs.
 */
// test -- use track.yaml and verify that we get the right parameters
EnsembleReader::EnsembleReader(
    const std::string deckNm, const std::string filename,
    djParameterList params,
    std::filesystem::path (*ensembleDirectoryPathOverride)(EnsembleReader*,
                                                           std::string,
                                                           djParameterList))
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
  std::string dimString = GetDimString();
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
  std::regex pattern("^" + config_prefix + R"(\.(\d+)$)");

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
