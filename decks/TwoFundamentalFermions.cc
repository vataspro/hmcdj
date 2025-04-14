//g++ --std=c++17 gridyaml.cpp -o test -I/opt/homebrew/Cellar/yaml-cpp/0.8.0/include/ -L/opt/homebrew/Cellar/yaml-cpp/0.8.0/lib -I/Users/alexi/Work/phd/GRID/prefix_grid_202410/include -L/Users/alexi/Work/phd/GRID/prefix_grid_202410/lib -I/Users/alexi/openssl/include -L/Users/alexi/openssl/lib -lGrid  -lyaml-cpp -lz
#include <hmcdj/utils/utils.h>
#include <Grid/Grid.h>

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

    // Initialise Grid
    Grid::Grid_init(&gridc, &gridv_ptr);

    /* typedefs */
    typedef Grid::Representations<Grid::SpFundamentalRepresentation> TheRepresentations;
    typedef Grid::SpWilsonImplR FermionImplPolicy;
    typedef Grid::SpWilsonFermionD FermionAction;
    typedef typename FermionAction::FermionField FermionField;

    // Print the layout
    Grid::GridLogLayout();

    // Instantiate the HMC wrapper
    typedef Grid::GenericSpHMCRunnerHirep<TheRepresentations, Grid::MinimumNorm2> HMCWrapper;
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
    srand(SeedRNG(argv[0]));
    Grid::RNGModuleParameters RNGpar;
    RNGpar.serial_seeds = GenSerialSeed();
    RNGpar.parallel_seeds = GenSerialSeed();
    TheHMC.Resources.SetRNGSeeds(RNGpar);


    /* Observables -- just plaquette for now */
    typedef Grid::PlaquetteMod<HMCWrapper::ImplPolicy> PlaqObs;
    TheHMC.Resources.AddObservable<PlaqObs>();

    /* Action */
    Grid::RealD beta = 6.95;
    Grid::RealD mass = -0.87;

    Grid::SpWilsonGaugeActionR Waction(beta);

    auto GridPtr = TheHMC.Resources.GetCartesian();
    auto GridRBPtr = TheHMC.Resources.GetRBCartesian();

    Grid::SpFundamentalRepresentation::LatticeField U(GridPtr);

    FermionAction FermOp(U, *GridPtr, *GridRBPtr, mass);

    Grid::ConjugateGradient<FermionField> CG(1.0e-8, 2000, false);

    Grid::TwoFlavourPseudoFermionAction<FermionImplPolicy> Nf2(FermOp, CG, CG);

    Nf2.is_smeared = false;

    Grid::ActionLevel<HMCWrapper::Field, TheRepresentations> Level1(1);
    Level1.push_back(&Nf2);

    Grid::ActionLevel<HMCWrapper::Field, TheRepresentations> Level2(4);
    Level2.push_back(&Waction);

    TheHMC.TheAction.push_back(Level1);
    TheHMC.TheAction.push_back(Level2);

    //Grid::ActionLevel<HMCWrapper::Field> Level1(1);
    //Level1.push_back(&Waction);
    //TheHMC.TheAction.push_back(Level1);

    /* HMC PARAMETERS */
    // HMC parameters MD parameters
    TheHMC.Parameters.MD.MDsteps = reader.MDsteps;
    TheHMC.Parameters.MD.trajL   = reader.trajL;
    // Trajectories & Thermalisations (no reject)
    TheHMC.Parameters.NoMetropolisUntil = reader.Thermalisations;
    TheHMC.Parameters.Trajectories = reader.Trajectories;
    // Starting Type
    TheHMC.Parameters.StartingType = reader.StartingType;


    /* RUN THE HMC */
    TheHMC.Run(); 
    /* FINALIZE GRID */
    Grid::Grid_finalize();  

    return 0;
}