#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/checkpoint.h>
#include <hmcdj/utils/parameter.h>
#include <hmcdj/utils/utils.h>

/* DJSuccessfulExit should always be zero in production code.
   It should only be set to a non-zero value from a test harness,
   to allow detection of unwanted exits. */
template <typename HMCWrapper, int DJSuccessfulExit = 0>
class DJ {
  // Create a type alias such that the LoadCheckpointer template function called
  // below will accept this type as a template parameter. The
  // ImplementationPolicy parameter will be specified by that function.
  template <typename ImplementationPolicy>
  using theCPModule =
      ILDGTimingCPModule<ImplementationPolicy, DJSuccessfulExit>;

 public:
  DJ(std::string deckName, int argc, char* argv[], 
                           djParameterList Parameters, bool reduce_group=false);
  DJ(std::string deckName, int argc, char* argv[]);
  EnsembleReader reader;
  HMCWrapper TheHMC;
  void Play();
  ~DJ();
};

const int DJ_NUM_EXTRA_ARGS = 2;
char** getGridArgv(int argc, char* argv[], const char* grid);

template <typename HMCWrapper, int DJSuccessfulExit>
DJ<HMCWrapper, DJSuccessfulExit>::DJ(std::string deckName, 
                                     int argc, char* argv[], 
                                     djParameterList Parameters,
                                     bool reduce_group)
    : reader(deckName, argc < 2 ? "(no filename specified)" : argv[1],
             Parameters) {
  // Ensure correct usage
  djGuard(argc, argv);

  /* GRID PURE GAUGE STARTS HERE */

  // we can infer the gauge group from HMCWrapper,
  // this lets hmcdj instantiate the correct IldgWriter.
  std::string group;
  if constexpr (std::is_same_v<typename HMCWrapper::ImplPolicy::GaugeGroup,
                               Grid::Sp<Grid::Nc>>) {
    std::cout << DJLogMessage
              << "GaugeGroup is Grid::Sp - "
              << "setting group to sp" << std::endl;
    group = "sp";
  } else if constexpr(std::is_same_v<typename HMCWrapper::ImplPolicy::GaugeGroup
                                    ,Grid::SU<Grid::Nc>>) {
    std::cout << DJLogMessage
              << "GaugeGroup is Grid::SU - "
              << "setting group to su" << std::endl;
    group = "su";
  } else {
    std::cout << DJLogError << "Can't infer gauge group from HMC Runner"
                            << std::endl;
    exit(1);
  }

  // Initialise Grid and print the layout
  // Subtract one as we remove the track filename
  int gridArgc = argc + DJ_NUM_EXTRA_ARGS - 1;
  char** gridArgv = getGridArgv(argc, argv, reader.GetDimStringPointer());
  Grid::Grid_init(&gridArgc, &gridArgv);
  Grid::GridLogLayout();

  // Add gauge field
  TheHMC.Resources.AddFourDimGrid("gauge");

  // Checkpointer definition
  Grid::CheckpointerParameters CPparams;
  CPparams.config_prefix = reader.config_prefix;
  CPparams.rng_prefix =
      reader.rng_prefix;  // perhaps saving the rng should be optional?
  CPparams.saveInterval = reader.saveInterval;
  CPparams.format = reader.format;
  CPparams.group  = group;
  CPparams.reduced_matrix  = reduce_group;

  if(CPparams.reduced_matrix) {
    std::cout << DJLogMessage << "Checkpointer using reduced format writer"
                              << std::endl;
  }
  else {
    std::cout << DJLogMessage << "Checkpointer not using reduced format writer"
                              << std::endl;
  }

  TheHMC.Resources.template LoadCheckpointer<theCPModule>(CPparams);

  /* Seeding the Grid RNG */
  RNGManager rng(argv[1]);  // Seed with a deterministic rng from the yaml file

  Grid::RNGModuleParameters RNGpar;
  RNGpar.serial_seeds = rng.GenerateGridRNGSeedString();
  RNGpar.parallel_seeds = rng.GenerateGridRNGSeedString();
  TheHMC.Resources.SetRNGSeeds(RNGpar);

  /* Observables -- just plaquette for now */
  typedef Grid::PlaquetteMod<typename HMCWrapper::ImplPolicy> PlaqObs;
  TheHMC.Resources.template AddObservable<PlaqObs>();

  // HMC parameters MD parameters
  TheHMC.Parameters.MD.MDsteps = reader.MDsteps;
  TheHMC.Parameters.MD.trajL = reader.trajL;

  // Trajectories & Thermalisations (no reject)
  TheHMC.Parameters.NoMetropolisUntil = reader.Thermalisations;
  TheHMC.Parameters.Trajectories = reader.Trajectories;

  // Set the starting type and starting trajectory
  TheHMC.Parameters.StartingType = reader.StartingType;

  // Get the starting trajectory
  TheHMC.Parameters.StartTrajectory = reader.StartingTrajectory;
}

// Overload for passing no parameters
template <typename HMCWrapper, int DJSuccessfulExit>
DJ<HMCWrapper, DJSuccessfulExit>::DJ(std::string deckName, int argc,
                                     char* argv[])
    : DJ(deckName, argc, argv, {}) {}

/* Play the track: Run the HMC */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::Play() {
  TheHMC.Run();
}

/* Destructor closes Grid */
template <typename HMCWrapper, int DJSuccessfulExit>
DJ<HMCWrapper, DJSuccessfulExit>::~DJ() {
  Grid::Grid_finalize();
}
