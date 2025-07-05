#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/parameter.h>
#include <hmcdj/utils/utils.h>

template <typename HMCWrapper>
class DJ {
 public:
  DJ(int argc, char* argv[], std::vector<ParameterBase*> Parameters);
  EnsembleReader reader;
  HMCWrapper TheHMC;
  void Play();
  ~DJ();
};

template <typename HMCWrapper>
DJ<HMCWrapper>::DJ(int argc, char* argv[],
                   std::vector<ParameterBase*> Parameters)
    : reader(argv[1], Parameters) {
  // Ensure correct usage
  djGuard(argc, argv);

  // Hacky way to provide Grid with "fake" command line arguments
  int gridc = 3;

  char* gridv[] = {(char*)argv[0], (char*)"--grid",
                   (char*)reader.GetDimStringPointer(), nullptr};

  // This is super ugly
  char** gridv_ptr = (char**)gridv;

  /* GRID PURE GAUGE STARTS HERE */

  // Initialise Grid and print the layout
  Grid::Grid_init(&gridc, &gridv_ptr);
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

  TheHMC.Resources.LoadNerscCheckpointer(CPparams);

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

/* Play the track: Run the HMC */
template <typename HMCWrapper>
void DJ<HMCWrapper>::Play() {
  TheHMC.Run();
}

/* Destructor closes Grid */
template <typename HMCWrapper>
DJ<HMCWrapper>::~DJ() {
  Grid::Grid_finalize();
}
