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
    std::ifstream file(argv[1]); // opens a filestream of argv[1]
    if (!file) {
        std::cerr << "File " <<  argv[1] << " does not exist!\n";
        std::exit(EXIT_FAILURE);
    }
}

/* 
    * RNGManager Constructor

    The RNG Manager initialises an instance of the sitmo
    RNG engine from Grid's source code. This is deterministic
    and should be compiler indipendent.

    Using the Seed method, the RNG engine is seeded from the
    filename when the class is initialised.
 */
RNGManager::RNGManager(std::string filename) : engine() {
    Seed(filename);
}

/* RNGManager Seeding */
void RNGManager::Seed(std::string filename ) {
    engine.seed((md5FileToInt(filename)));
}

/* Generate Grid RNG string */
std::string RNGManager::GenerateGridRNGSeedString() {

    std::ostringstream RNGstr;
    std::uniform_int_distribution<int> dist(0, 100);

    RNGstr << dist(engine);

    for (int i=1; i<5; i++) {
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

    while (file.read(reinterpret_cast<char*>(buffer), bufferSize) || file.gcount() > 0) {
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
    std::memcpy(&result, md5Digest, sizeof(result)); // Use first 4 bytes
    return result;
}


/* 
 * Ensemble Reader Initialisation 
    Read an HMCDJ yaml file.

    Load the Grid job parameters.

    Check that the requested contents exist and raises
    and error if an issue occurs.
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
        config_prefix = track["checkpoint"]["configurations"]["prefix"].as<std::string>();
        rng_prefix = track["checkpoint"]["rng"]["prefix"].as<std::string>();

        /* HMC Parameters */
        trajL = track["HMC"]["MD"]["trajL"].as<double>();
        MDsteps = track["HMC"]["MD"]["MDsteps"].as<int>();

        Thermalisations = track["HMC"]["Thermalisations"].as<int>();
        Trajectories = track["HMC"]["Trajectories"].as<int>();
        StartingType = track["HMC"]["StartingType"].as<std::string>();


    } catch (const YAML::Exception &e) { // protect against mistake in yaml file
        std::cerr << "Error loading yaml file: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }

}

// ** make a test
/*
 * Grid Dimensions String
    Returns a Grid-readable string (pointer to char) to initialise the
    lattice 4-volume from the yaml file.
*/
const char* EnsembleReader::GetDimStringPointer() {
        dimString = std::to_string(nx) + "." +
                    std::to_string(ny) + "." +
                    std::to_string(nz) + "." +
                    std::to_string(nt);

        return dimString.c_str();
    }
