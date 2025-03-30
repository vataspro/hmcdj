//g++ --std=c++17 gridyaml.cpp -o test -I/opt/homebrew/Cellar/yaml-cpp/0.8.0/include/ -L/opt/homebrew/Cellar/yaml-cpp/0.8.0/lib -I/Users/alexi/Work/phd/GRID/prefix_grid_202410/include -L/Users/alexi/Work/phd/GRID/prefix_grid_202410/lib -I/Users/alexi/openssl/include -L/Users/alexi/openssl/lib -lGrid  -lyaml-cpp -lz
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <yaml-cpp/yaml.h> // yaml-cpp is required
#include <Grid/Grid.h>
#include <functional> // can be exchanged for ssh when I go to MD5 for the hash
#include <sstream>

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
int seedRNG(const std::string& name) {
    std::string filename = name;
    
    return std::hash<std::string>{}(filename);
}

std::string genSerialSeed() {

    std::ostringstream RNGstr;
    for (int i=0; i<5; i++) {
        if (i > 0) RNGstr << " ";

        RNGstr << std::to_string(rand());
    }

    return RNGstr.str();
}

/*
 * Ensemble Reader class

 This class reads the provided yaml file and stores the required
 parameter values.
*/
class EnsembleReader{
  private:
    /* Lattice dimensions */
    int nt, nx, ny, nz; // Lattice dims
    std::string dimString; // Lattice dimensions string

  public:
    /* Checkpointing */
    int saveInterval;
    std::string config_prefix, 
                rng_prefix,
                format;
    /* Action parameters */
    double beta;
    /* HMC parameters */
    double trajL;
    int MDsteps, Thermalisations, Trajectories;
    std::string StartingType;
    

    // Constructor - reads and loads the parameters from the yaml file
    EnsembleReader(const std::string filename) {

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

            SetDimString(); // String for Grid_init

            /* CHECKPOINTING */
            saveInterval = track["checkpoint"]["saveInterval"].as<int>();
            format = track["checkpoint"]["format"].as<std::string>();
            config_prefix = track["checkpoint"]["configurations"]["prefix"].as<std::string>();
            rng_prefix = track["checkpoint"]["rng"]["prefix"].as<std::string>();

            /* ACTION */
            beta = track["Action"]["beta"].as<double>();

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

    /* GRID PURE GAUGE STARTS HERE */

    // Initialise Grid and print the layout
    Grid::Grid_init(&gridc, &gridv_ptr);
    Grid::GridLogLayout();

    // Instantiate the HMC wrapper
    typedef Grid::GenericSpHMCRunner<Grid::MinimumNorm2> HMCWrapper;
    HMCWrapper TheHMC;
    // Add gauge field
    TheHMC.Resources.AddFourDimGrid("gauge");

    // Checkpointer definition
    Grid::CheckpointerParameters CPparams;  
    CPparams.config_prefix = reader.config_prefix;
    CPparams.rng_prefix = reader.rng_prefix; // perhaps saving the rng should be optional?
    CPparams.saveInterval = reader.saveInterval;
    CPparams.format = reader.format;

    TheHMC.Resources.LoadNerscCheckpointer(CPparams);


    /* Seeding the RNG */
    /* An if statement can check whether the ckpointer
        has loaded the rng from the previous run.
    */
    // For seeding the RNG
    srand(seedRNG(argv[0]));

    Grid::RNGModuleParameters RNGpar;
    RNGpar.serial_seeds = genSerialSeed();
    RNGpar.parallel_seeds = genSerialSeed();
    TheHMC.Resources.SetRNGSeeds(RNGpar);


    /* Observables -- just plaquette for now */
    typedef Grid::PlaquetteMod<HMCWrapper::ImplPolicy> PlaqObs;
    TheHMC.Resources.AddObservable<PlaqObs>();

    /* Action */
    Grid::SpWilsonGaugeActionR Waction((Grid::RealD)reader.beta);
  
    Grid::ActionLevel<HMCWrapper::Field> Level1(1);
    Level1.push_back(&Waction);
    TheHMC.TheAction.push_back(Level1);

    // HMC parameters MD parameters
    TheHMC.Parameters.MD.MDsteps = reader.MDsteps;
    TheHMC.Parameters.MD.trajL   = reader.trajL;

    // Trajectories & Thermalisations (no reject)
    TheHMC.Parameters.NoMetropolisUntil = reader.Thermalisations;
    TheHMC.Parameters.Trajectories = reader.Trajectories;

    TheHMC.Parameters.StartingType = reader.StartingType;


    // TheHMC.ReadCommandLine(argc, argv); // these can be parameters from file
    for (int i=0; i<1; i++) // Using this just reseeds the HMC and produces the same result :(
        TheHMC.Run();  // no smearing



    Grid::Grid_finalize();  

    return 0;
}