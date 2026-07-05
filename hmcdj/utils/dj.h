#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/checkpoint.h>
#include <hmcdj/utils/mathutils.h>
#include <hmcdj/utils/parameter.h>
#include <hmcdj/utils/utils.h>

#include <filesystem>

#include "acceptance.h"

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

 public:
  DJ(std::string deckName, int argc, char* argv[], djParameterList Parameters,
     bool reduce_group = false);
  DJ(std::string deckName, int argc, char* argv[]);
  EnsembleReader reader;
  HMCWrapper TheHMC;
  void Tune();
  void Play();

  // AcceptanceObsParameters AccPar;  // Acceptance rate tuning parameters
  CheckpointerExtraParams extraCPPars;

  // Tuning functions
  void loadTuningState();
  void setupTuningStep();
  void endTuningStep();
  void tuneAcceptance();
  void setTuningTrajectories();
  void finaliseTuning();

  // Destructor
  ~DJ();
};

const int DJ_NUM_EXTRA_ARGS = 2;
char** getGridArgv(int argc, char* argv[], const char* grid);

template <typename HMCWrapper, int DJSuccessfulExit>
DJ<HMCWrapper, DJSuccessfulExit>::DJ(std::string deckName, int argc,
                                     char* argv[], djParameterList Parameters,
                                     bool reduce_group)
    : reader(deckName, argc < 2 ? "(no filename specified)" : argv[1],
             Parameters) {
  // Ensure correct usage
  djGuard(argc, argv);

  // we can infer the gauge group from HMCWrapper,
  // this lets hmcdj instantiate the correct IldgWriter.
  std::string group;
  if constexpr (std::is_same_v<typename HMCWrapper::ImplPolicy::GaugeGroup,
                               Grid::Sp<Grid::Nc>>) {
    std::cout << DJLogMessage << "GaugeGroup is Grid::Sp - "
              << "setting group to sp" << std::endl;
    group = "sp";
  } else if constexpr (std::is_same_v<
                           typename HMCWrapper::ImplPolicy::GaugeGroup,
                           Grid::SU<Grid::Nc>>) {
    std::cout << DJLogMessage << "GaugeGroup is Grid::SU - "
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
  CPparams.group = group;
  CPparams.reduced_matrix = reduce_group;

  if (CPparams.reduced_matrix) {
    std::cout << DJLogMessage << "Checkpointer using reduced format writer"
              << std::endl;
  } else {
    std::cout << DJLogMessage << "Checkpointer not using reduced format writer"
              << std::endl;
  }

  // TheHMC.Resources.CP.Params.AccParams = &AccPar;
  // TheHMC.Resources.CP->setAccParams(&AccPar);

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
  extraCPPars.AccPar->total_num_init_skips =
      reader.total_num_init_skips;  // Number of parameters to skip from
  extraCPPars.AccPar->num_tuning_samples =
      reader.num_tuning_samples;  // Number of samples to tune for
  extraCPPars.AccPar->target_rate =
      reader.target_rate;  // Target acceptance rate
  extraCPPars.AccPar->target_rate_tol =
      reader.target_rate_tol;  // Acceptance rate tuning tolerance
  extraCPPars.AccPar->monitor_every = reader.monitor_every;
  extraCPPars.AccPar->MDsteps =
      &TheHMC.Parameters.MD.MDsteps;  // required for saving mdsteps

  typedef AcceptanceMod<typename HMCWrapper::ImplPolicy> AccObs;
  TheHMC.Resources.template AddObservable<AccObs>(*extraCPPars.AccPar);

  extraCPPars.AccPar = extraCPPars.AccPar;

  // Add Checkpointer as last observable
  TheHMC.Resources.template LoadCheckpointer<theCPModule>(CPparams,
                                                          extraCPPars);

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

/* Acceptance Rate Tuning */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::Tune() {
  loadTuningState();  // Load or initialise the tuning
  while (*extraCPPars.AccPar->tuning_mode != tuning_mode_t::complete) {
    setupTuningStep();  // Set trajectory number
    Play();  // Will run up to exactly after next tuning step is reached
    endTuningStep();
  }
  // Ensure that correct values are loaded for trajectories when going to Play()
  finaliseTuning();
}

/*
   Load or initialise the tuning state
   Checks for a tuning file; if there is one, load its state.
   If none is present, tuning is definitely still initialising.
 */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::loadTuningState() {
  if (std::filesystem::exists(extraCPPars.AccPar->tuning_filename)) {
    // Tuning has already started
    Grid::XmlReader TuningReader(extraCPPars.AccPar->tuning_filename);
    // Read current tuning mode
    int tmp_buf;
    TuningReader.readDefault("mode", tmp_buf);
    *extraCPPars.AccPar->tuning_mode = static_cast<tuning_mode_t>(tmp_buf);
    // Load current MDsteps value
    TuningReader.readDefault("MDsteps", TheHMC.Parameters.MD.MDsteps);
    // Load tuning counter
    TuningReader.readDefault("tuning_ctr", *extraCPPars.AccPar->tuning_ctr);

  } else {
    // First run; initialise tuning
    *extraCPPars.AccPar->tuning_mode = tuning_mode_t::init;
    *extraCPPars.AccPar->tuning_ctr = 0;
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
  This function will only be reached if the tuning is
  not complete.
 */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::setTuningTrajectories() {
  // Initialisation mode
  if (*extraCPPars.AccPar->tuning_mode == tuning_mode_t::init) {
    TheHMC.Parameters.Trajectories = +extraCPPars.AccPar->total_num_init_skips -
                                     TheHMC.Parameters.StartTrajectory;

  } else {  // This should only be reached if tuning mode is == active
    int num_valid_traj = TheHMC.Parameters.StartTrajectory -
                         extraCPPars.AccPar->total_num_init_skips;
    TheHMC.Parameters.Trajectories =
        extraCPPars.AccPar->num_tuning_samples -
        (num_valid_traj % extraCPPars.AccPar->num_tuning_samples);
  }
}

/*
    Set the trajectories for the next tuning step.
    Run the acceptance tuning.
    This function will only be reached once the tuning
    is active.
 */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::endTuningStep() {
  // Increment trajectories
  TheHMC.Parameters.StartTrajectory += TheHMC.Parameters.Trajectories;

  if (*extraCPPars.AccPar->tuning_mode ==
      tuning_mode_t::init) {  // activate tuning

    *extraCPPars.AccPar->tuning_mode = tuning_mode_t::active;  // on first pass

    // Remove NoMetropolis step on first pass
    TheHMC.Parameters.StartTrajectory += TheHMC.Parameters.NoMetropolisUntil;
    TheHMC.Parameters.NoMetropolisUntil = 0;
    TheHMC.Parameters.StartingType = "CheckpointStart";
  } else if (*extraCPPars.AccPar->tuning_mode == tuning_mode_t::active) {
    tuneAcceptance();  // tune acceptance rate
  }

  // Iterate acceptance tuning step counter
  ++(*extraCPPars.AccPar->tuning_ctr);

  // Save acceptance tuning state
  Grid::XmlWriter TuningWriter(extraCPPars.AccPar->tuning_filename);
  write(TuningWriter, "mode",
        static_cast<int>(*extraCPPars.AccPar->tuning_mode));
  write(TuningWriter, "tuning_ctr", *extraCPPars.AccPar->tuning_ctr);
  write(TuningWriter, "MDsteps", *extraCPPars.AccPar->MDsteps);

  Grid::XmlWriter AccWriter(extraCPPars.AccPar->acceptance_filename);
  write(AccWriter, "acc", *extraCPPars.AccPar->AcceptanceArray);
}

template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::tuneAcceptance() {
  // Measure acceptance rate
  double pacc = mean(*extraCPPars.AccPar->AcceptanceArray);
  double pacc_err = stdErr(*extraCPPars.AccPar->AcceptanceArray);
  // Empty the acceptance array
  extraCPPars.AccPar->AcceptanceArray->clear();

  std::cout << Grid::GridLogMessage << "Current acceptance rate is: " << pacc
            << " +/- " << pacc_err << std::endl;

  if (fabs(pacc - extraCPPars.AccPar->target_rate) <
      extraCPPars.AccPar->target_rate_tol) {
    // Tuning complete
    *extraCPPars.AccPar->tuning_mode = tuning_mode_t::complete;
    std::cout << Grid::GridLogMessage
              << "Tuning successful, with final MD steps: "
              << TheHMC.Parameters.MD.MDsteps << std::endl;

    // Save tuning final state
    Grid::XmlWriter TuningWriter(extraCPPars.AccPar->tuning_filename);
    write(TuningWriter, "mode", static_cast<int>(tuning_mode_t::complete));
    write(TuningWriter, "tuning_ctr", extraCPPars.AccPar->tuning_ctr);
    write(TuningWriter, "MDsteps", TheHMC.Parameters.MD.MDsteps);

  } else {  // Bad acceptance rate - tune MD steps

    // TODO: Make safe for 0/inf
    int target_MD = get_target_MDsteps(TheHMC.Parameters.MD.trajL,
                                       TheHMC.Parameters.MD.MDsteps, pacc,
                                       extraCPPars.AccPar->target_rate);

    std::cout << Grid::GridLogMessage
              << "Best approximation for target MDsteps: " << target_MD
              << " with Dt: "
              << static_cast<double>(TheHMC.Parameters.MD.trajL) / target_MD
              << std::endl;

    TheHMC.Parameters.MD.MDsteps = target_MD;
  }
}

/*
    Do any final things after the tuning.
 */
template <typename HMCWrapper, int DJSuccessfulExit>
void DJ<HMCWrapper, DJSuccessfulExit>::finaliseTuning() {
  TheHMC.Parameters.Trajectories = reader.Trajectories;
  std::cout << Grid::GridLogMessage << "Acceptance rate tuning complete."
            << std::endl;
}

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
