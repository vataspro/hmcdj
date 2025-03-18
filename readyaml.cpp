#include <iostream>
#include <fstream>
#include <cstdlib>
#include <yaml-cpp/yaml.h>

/*
 * Guard

 Ensures that the programme is called correctly
*/
void djGuard(int argc, char* argv[]) {

    // Usage
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << "filename\n";
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

/*
 * Ensemble Reader class

 This class reads the provided yaml file and stores the required
 parameter values.
*/
class EnsembleReader{
  private:
    int nt, nx, ny, nz; // Lattice dims

  public:
    // Constructor - reads and loads the parameters from the yaml file
    EnsembleReader(const std::string filename) {

        // Load the parameters from the yaml file
        try {
            // Load the yaml file
            YAML::Node track = YAML::LoadFile(filename);

            // Grid lattice parameters
            nt = track["volume"]["nt"].as<int>();
            nx = track["volume"]["nx"].as<int>();
            ny = track["volume"]["ny"].as<int>();
            nz = track["volume"]["nz"].as<int>();

        } catch (const YAML::Exception &e) { // protect against bad yaml file
            std::cerr << "Error loading yaml file: " << e.what() << "\n";
            exit(EXIT_FAILURE);
        }
    }

    // Get the number of points for a particular lattice dimension
    int dimLength(const int i) {
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
};

/* 
 * MAIN
 */
int main(int argc, char* argv[]) {

    // Ensure correct usage
    djGuard(argc, argv);

    // Read Grid parameters from the input file
    EnsembleReader reader(argv[1]);

    // Test output
    std::cout << reader.dimLength(0) << "\n";

    return 0;
}