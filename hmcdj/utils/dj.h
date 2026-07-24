#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/acceptance.h>
#include <hmcdj/utils/checkpoint.h>
#include <hmcdj/utils/logging.h>
#include <hmcdj/utils/mathutils.h>
#include <hmcdj/utils/parameter.h>
#include <hmcdj/utils/utils.h>

#include <filesystem>

/* DJSuccessfulExit should always be zero in production code.
   It should only be set to a non-zero value from a test harness,
   to allow detection of unwanted exits. */
template <typename HMCWrapper, int DJSuccessfulExit = 0>
class DJ {
  // Create a type alias such that the LoadCheckpointer template function called
  // below will accept this type as a template parameter. The
  // ImplementationPolicy parameter will be specified by that function.
  template <typename ImplementationPolicy, typename ExtraCPParams>
  using theCPModule =
      ILDGTimingCPModule<ImplementationPolicy, ExtraCPParams, DJSuccessfulExit>;
  teestdout tee;

 public:
  DJ(std::string deckName, int argc, char* argv[], djParameterList Parameters,
     bool useReducedStorage = false,
     pathCallback ensembleDirectoryPathOverride = nullptr);
  DJ(std::string deckName, int argc, char* argv[],
     bool useReducedStorage = false,
     pathCallback ensembleDirectoryPathOverride = nullptr);
  EnsembleReader reader;
  HMCWrapper TheHMC;
  void Play();

  // Any extra parameters needed at checkpoint time
  CheckpointerExtraParams extraCPPars;

  // Tuning functions
  void setupTuningStep();
  void endTuningStep();
  void tuneAcceptance(bool adjust = true);
  void setTuningTrajectories();

  // Destructor
  ~DJ();
};

const int DJ_NUM_EXTRA_ARGS = 2;
char** getGridArgv(int argc, char* argv[], const char* grid);

template <typename HMCWrapper, int DJSuccessfulExit>
DJ<HMCWrapper, DJSuccessfulExit>::DJ(std::string deckName, int argc,
                                     char* argv[], djParameterList Parameters,
                                     bool useReducedStorage,
                                     pathCallback ensembleDirectoryPathOverride)
    : reader(deckName, (djGuard(argc, argv), argv[1]), Parameters,
             ensembleDirectoryPathOverride) {
  // Using the comma operator in the line above
  // (`(djGuard(argv, argv), argv[1])`)
  // allows guarding against incorrect usage (including calling without
  // arguments) while maintaining `const` attributes.

  // Initialise Grid and print the layout
  // Subtract one as we remove the track filename
  int gridArgc = argc + DJ_NUM_EXTRA_ARGS - 1;
  char** gridArgv = getGridArgv(argc, argv, reader.GetDimStringPointer());
  Grid::Grid_init(&gridArgc, &gridArgv);
  setUpLogging(reader.EnsembleDirectory, tee);

  // Add gauge field
  TheHMC.Resources.AddFourDimGrid("gauge");

  // Checkpointer definition
  Grid::CheckpointerParameters CPparams;
  CPparams.config_prefix = reader.config_prefix;
  CPparams.rng_prefix =
      reader.rng_prefix;  // perhaps saving the rng should be optional?
  CPparams.saveInterval = reader.saveInterval;
  CPparams.format = reader.format;
  CPparams.group =
      getGaugeGroupString<typename HMCWrapper::ImplPolicy::GaugeGroup>();
  CPparams.reduced_matrix = useReducedStorage;

  if (CPparams.reduced_matrix) {
    std::cout << DJLogMessage << "Checkpointer using reduced format writer"
              << std::endl;
  } else {
    std::cout << DJLogMessage << "Checkpointer using full-matrix format writer"
              << std::endl;
  }

  /* Seeding the Grid RNG */
  RNGManager rng(argv[1]);  // Seed with a deterministic rng from the yaml file

  Grid::RNGModuleParameters RNGpar;
  RNGpar.serial_seeds = rng.GenerateGridRNGSeedString();
  RNGpar.parallel_seeds = rng.GenerateGridRNGSeedString();
  TheHMC.Resources.SetRNGSeeds(RNGpar);

  /* Observables -- In a standard HMCDJ programme only the plaquette is printed
   */
  typedef Grid::PlaquetteMod<typename HMCWrapper::ImplPolicy> PlaqObs;
  TheHMC.Resources.template AddObservable<PlaqObs>();

  // Acceptance rate & tuning
  extraCPPars.acceptance =
      std::make_shared<AcceptanceObsParameters>(reader, &TheHMC.Parameters.MD);

  typedef AcceptanceMod<typename HMCWrapper::ImplPolicy> AccObs;
  TheHMC.Resources.template AddObservable<AccObs>(*extraCPPars.acceptance);

  // Add Checkpointer as last observable
  TheHMC.Resources.template LoadCheckpointer<theCPModule>(CPparams,
                                                          extraCPPars);
  // HMC parameters MD parameters
  TheHMC.Parameters.MD.trajL = reader.trajL;

  // Set the starting type and starting trajectory
  TheHMC.Parameters.StartingType = reader.StartingType;

  // Get the starting trajectory
  TheHMC.Parameters.StartTrajectory = reader.StartingTrajectory;
}

// Overload for passing no parameters
template <typename HMCWrapper, int DJSuccessfulExit>
DJ<HMCWrapper, DJSuccessfulExit>::DJ(std::string deckName, int argc,
                                     char* argv[], bool useReducedStorage,
                                     pathCallback ensembleDirectoryPathOverride)
    : DJ(deckName, argc, argv, {}, useReducedStorage,
         ensembleDirectoryPathOverride) {}

/* Acceptance Rate Tuning */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::Play() {
  while (extraCPPars.acceptance->tuningMode() != tuning_mode_t::complete) {
    setupTuningStep();  // Set trajectory number
    TheHMC.Run();  // Will run up to exactly after next tuning step is reached
    endTuningStep();
  }
}

/*
    Set up tuning step
 */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::setupTuningStep() {
  setTuningTrajectories();
}

/*
  Set the number of trajectories required for tuning
  up to the next tuning step.
 */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::setTuningTrajectories() {
  const int currentTrajectory = extraCPPars.acceptance->currentTrajectory();
  const int thermalisationTrajectories =
      extraCPPars.acceptance->thermalisationTrajectories;
  TheHMC.Parameters.Trajectories =
      extraCPPars.acceptance->trajectoriesToNextTune();
  if (currentTrajectory < thermalisationTrajectories) {
    TheHMC.Parameters.NoMetropolisUntil =
        thermalisationTrajectories - currentTrajectory;
  } else {
    TheHMC.Parameters.NoMetropolisUntil = 0;
  }
  std::cout << DJLogDebug
            << "Next tuning/monitoring cycle starting; Trajectories: "
            << TheHMC.Parameters.Trajectories
            << "; Thermalisations: " << TheHMC.Parameters.NoMetropolisUntil
            << std::endl;
}

/*
    Set the trajectories for the next tuning step.
    Run the acceptance tuning.
 */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::endTuningStep() {
  // Increment trajectories
  TheHMC.Parameters.StartTrajectory =
      extraCPPars.acceptance->currentTrajectory();

  const tuning_mode_t tuningMode = extraCPPars.acceptance->tuningMode();
  switch (tuningMode) {
    case tuning_mode_t::monitoring:
      tuneAcceptance(false);
      break;
    case tuning_mode_t::active:
      tuneAcceptance();
      break;
    default:
      std::cout << DJLogDebug << "Tuning is "
                << tuningModeDescription[tuningMode] << ", taking no action."
                << std::endl;
  }
}

template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::tuneAcceptance(bool adjust) {
  // Measure acceptance rate
  const NumberWithError<double> pacc = extraCPPars.acceptance->avgAcceptance();
  const NumberWithError<double> paccClamped =
      extraCPPars.acceptance->avgAcceptance(true);

  std::cout << DJLogMessage << "Current acceptance rate is: " << pacc.value
            << " +/- " << pacc.error << std::endl;

  if (pacc.isClose(extraCPPars.acceptance->targetAcceptance,
                   extraCPPars.acceptance->deltaTargetAcceptance)) {
    std::cout << DJLogMessage
              << "Acceptance within desired range; taking no action."
              << std::endl;
  } else {
    // Acceptance rate not within tolerance range
    if (adjust) {
      // tune MD steps
      const int nextMDsteps = get_target_MDsteps(
          TheHMC.Parameters.MD.trajL, TheHMC.Parameters.MD.MDsteps,
          paccClamped.value, extraCPPars.acceptance->targetAcceptance);

      std::cout << DJLogMessage
                << "Best approximation for target MDsteps: " << nextMDsteps
                << " with Dt: "
                << static_cast<double>(TheHMC.Parameters.MD.trajL) / nextMDsteps
                << std::endl;

      extraCPPars.acceptance->MDsteps(nextMDsteps);
    } else {
      std::cout << DJLogError
                << "Acceptance outside prescribed limits in production!"
                << std::endl;
    }
  }
}

/* Destructor closes Grid */
template <typename HMCWrapper, int DJSuccessfulExit>
DJ<HMCWrapper, DJSuccessfulExit>::~DJ() {
  Grid::Grid_finalize();
}
