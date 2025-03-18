//g++ --std=c++17 gridyaml.cpp -o test -I/opt/homebrew/Cellar/yaml-cpp/0.8.0/include/ -L/opt/homebrew/Cellar/yaml-cpp/0.8.0/lib -I/Users/alexi/Work/phd/GRID/prefix_grid_202410/include -L/Users/alexi/Work/phd/GRID/prefix_grid_202410/lib -I/Users/alexi/openssl/include -L/Users/alexi/openssl/lib -lGrid  -lyaml-cpp
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <yaml-cpp/yaml.h> // yaml-cpp is required
#include <Grid/Grid.h>

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

/*
 * Ensemble Reader class

 This class reads the provided yaml file and stores the required
 parameter values.
*/
class EnsembleReader{
  private:
    int nt, nx, ny, nz; // Lattice dims
    std::string dimString;

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

            SetDimString();

        } catch (const YAML::Exception &e) { // protect against bad yaml file
            std::cerr << "Error loading yaml file: " << e.what() << "\n";
            exit(EXIT_FAILURE);
        }
    }

    // Get the number of points in a particular lattice dimension
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

    // Set the Grid dimensions string
    void SetDimString() {
        dimString = std::to_string(dimLength(0)) + "." +
                    std::to_string(dimLength(1)) + "." +
                    std::to_string(dimLength(2)) + "." +
                    std::to_string(dimLength(3));
    }

    // Gets a char pointer to the dimension string
    const char* GetDimStringPointer() {
        return dimString.c_str();
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

    // Hacky way to provide Grid with "fake" command line arguments
    int gridc = 3;

    char* gridv[] = {
        (char*)argv[0],
        (char*)"--grid",
        (char*)reader.GetDimStringPointer(),
        nullptr
    };

    // This is super ugly
    char** gridv_ptr = (char**) gridv;

    // Initialise Grid and print the layout
    Grid::Grid_init(&gridc, &gridv_ptr);
    Grid::GridLogLayout();
    Grid::Grid_finalize();  

    return 0;
}