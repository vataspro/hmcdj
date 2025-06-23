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
    std::cerr << "Usage: " << argv[0] << " filename --<Other Grid arguments>\n";
    std::exit(EXIT_FAILURE);
  }

  // Check that the file exists
  std::ifstream file(argv[1]);  // opens a filestream of argv[1]
  if (!file) {
    std::cerr << "File " << argv[1] << " does not exist!\n";
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

  uint32_t result;
  std::memcpy(&result, md5Digest, sizeof(result));  // Use first 4 bytes
  return result;
}

/*
 * Ensemble Reader Initialisation
    Read an HMCDJ yaml file.

    Load the Grid job parameters.

    Check that the requested contents exist and raise
    an error if an issue occurs.
 */
// test -- use track.yaml and verify that we get the right parameters
EnsembleReader::EnsembleReader(const std::string filename) {
  // Load the parameters from the yaml file
  try {
    // Load the yaml file
    YAML::Node track = YAML::LoadFile(filename);

    /* VOLUME */
    // Grid lattice parameters
    nt = track["volume"]["nt"].as<int>();
    nx = track["volume"]["nx"].as<int>();
    ny = track["volume"]["ny"].as<int>();
    nz = track["volume"]["nz"].as<int>();

    /* CHECKPOINTING */
    saveInterval = track["checkpoint"]["saveInterval"].as<int>();
    format = track["checkpoint"]["format"].as<std::string>();
    config_prefix =
        track["checkpoint"]["configurations"]["prefix"].as<std::string>();
    rng_prefix = track["checkpoint"]["rng"]["prefix"].as<std::string>();

    /* HMC Parameters */
    trajL = track["HMC"]["MD"]["trajL"].as<double>();
    MDsteps = track["HMC"]["MD"]["MDsteps"].as<int>();

    Thermalisations = track["HMC"]["Thermalisations"].as<int>();
    Trajectories = track["HMC"]["Trajectories"].as<int>();
    StartingType = track["HMC"]["StartingType"].as<std::string>();

    /* HMCDJ Parameters */
    EnsembleDirectory = track["HMCDJ"]["EnsembleDirectory"].as<std::string>();

    /* Dynamic Start */
    setStart();  // Choose the StartingType and StartingTrajectory dynamically
                 // by checking the enseble home directory for configurations

  } catch (const YAML::Exception& e) {  // protect against mistake in yaml file
    std::cerr << "Error loading yaml file: " << e.what() << "\n";
    exit(EXIT_FAILURE);
  }
}

/*
 * Grid Dimensions String
    Returns a Grid-readable string (pointer to char) to initialise the
    lattice 4-volume from the yaml file.
*/
const char* EnsembleReader::GetDimStringPointer() {
  dimString = std::to_string(nx) + "." + std::to_string(ny) + "." +
              std::to_string(nz) + "." + std::to_string(nt);

  return dimString.c_str();
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
void EnsembleReader::setStart() {
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
       std::filesystem::directory_iterator(EnsembleDirectory)) {
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
  }
}
