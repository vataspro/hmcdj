//g++ --std=c++17 gridyaml.cpp -o test -I/opt/homebrew/Cellar/yaml-cpp/0.8.0/include/ -L/opt/homebrew/Cellar/yaml-cpp/0.8.0/lib -I/Users/alexi/Work/phd/GRID/prefix_grid_202410/include -L/Users/alexi/Work/phd/GRID/prefix_grid_202410/lib -I/Users/alexi/openssl/include -L/Users/alexi/openssl/lib -lGrid  -lyaml-cpp -lz
#include "deck.h"
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