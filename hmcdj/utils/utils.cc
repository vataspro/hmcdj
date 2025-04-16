#include <hmcdj/utils/utils.h>

/*
 * Guard

 Ensures that the programme is called correctly
*/
void djGuard(int argc, char* argv[]) {

    // Usage
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " filename\n";
        std::exit(EXIT_FAILURE);
    }

    // Check that the file exists
    std::ifstream file(argv[1]); // opens a filestream of argv[1]
    if (!file) {
        std::cerr << "File " <<  argv[1] << " does not exist!\n";
        std::exit(EXIT_FAILURE);
    }

    // Perhaps also check that the called file is a yaml file?
}

/* Ed's magic function to seed an rng 

    Since openssl is required by Grid,
    this can be re-written using md5
    (which is part of openssl).
*/
// TODO!!! Not a good name for this function
/* For testing, verify for a known string that the 
generated hash is consistent */
int SeedRNG(const std::string& name) {

    // check why this line is here
    std::string filename = name;
    
    return std::hash<std::string>{}(filename);
}

// docstring
// argument should be a seeded rng
// test -- pass a known seeded state and
// check that I get the right things, number of spaces etc..
std::string GenSerialSeed() {

    std::ostringstream RNGstr;
    for (int i=0; i<5; i++) {
        if (i > 0) RNGstr << " ";

        RNGstr << std::to_string(rand()); // seed the rng?!?
    }

    return RNGstr.str();
}


/* Ensemble Reader Initialisation */
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

        /* ACTION */
        //beta = track["Action"]["beta"].as<double>();

        /* HMC Parameters */
        trajL = track["HMC"]["MD"]["trajL"].as<double>();
        MDsteps = track["HMC"]["MD"]["MDsteps"].as<int>();

        Thermalisations = track["HMC"]["Thermalisations"].as<int>();
        Trajectories = track["HMC"]["Trajectories"].as<int>();
        StartingType = track["HMC"]["StartingType"].as<std::string>();


    } catch (const YAML::Exception &e) { // protect against bad yaml file
            std::cerr << "Error loading yaml file: " << e.what() << "\n";
            exit(EXIT_FAILURE);
        }
    }

// Remove!
// Get the number of points in a particular lattice dimension
int EnsembleReader::DimLength(const int i) {
        switch (i) {
            case 0: return nx;
            case 1: return ny;
            case 2: return nz;
            case 3: return nt;
            default:{
                std::cerr << "Error! Dimension must be between 0 and 3.\n";
                exit(EXIT_FAILURE);
            }
        }
    }

// Gets a char pointer to the dimension string
// ** delete lines 100-113
// ** replace DimLength(0) with nx, ... etc
// ** make a test
const char* EnsembleReader::GetDimStringPointer() {
        dimString = std::to_string(DimLength(0)) + "." +
                    std::to_string(DimLength(1)) + "." +
                    std::to_string(DimLength(2)) + "." +
                    std::to_string(DimLength(3));

        return dimString.c_str();
    }
