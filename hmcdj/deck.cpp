#include "deck.h"

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

    // Get the number of points in a particular lattice dimension
int EnsembleReader::dimLength(const int i) {
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
const char* EnsembleReader::GetDimStringPointer() {
        dimString = std::to_string(dimLength(0)) + "." +
                    std::to_string(dimLength(1)) + "." +
                    std::to_string(dimLength(2)) + "." +
                    std::to_string(dimLength(3));

        return dimString.c_str();
    }
