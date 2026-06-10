#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/mathutils.h>
#include <hmcdj/utils/parameter.h>
#include <hmcdj/utils/utils.h>

#include <filesystem>

#include "acceptance.h"

template <typename HMCWrapper>
class DJ {
 public:
  DJ(std::string deckName, int argc, char* argv[], djParameterList Parameters);
  DJ(std::string deckName, int argc, char* argv[]);
  EnsembleReader reader;
  HMCWrapper TheHMC;
  void Tune();
  void Play();

  AcceptanceObsParameters AccPar;  // Acceptance rate tuning parameters

  ~DJ();
};

const int DJ_NUM_EXTRA_ARGS = 2;
char** getGridArgv(int argc, char* argv[], const char* grid);

template <typename HMCWrapper>
DJ<HMCWrapper>::DJ(std::string deckName, int argc, char* argv[],
                   djParameterList Parameters)
    : reader(deckName, argc < 2 ? "(no filename specified)" : argv[1],
             Parameters) {
  // Ensure correct usage
  djGuard(argc, argv);

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

  TheHMC.Resources.LoadNerscCheckpointer(CPparams);

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
  // TODO: read from track
  AccPar.rethermalisation = 10;    // Number of parameters to skip from
  AccPar.num_tuning_samples = 50;  // Number of samples to tune for
  AccPar.target_rate = 0.8;        // Target acceptance rate
  AccPar.target_rate_flex = 0.05;  // Flexibiility of acceptance rate
  AccPar.monitor_every = 100;
  typedef AcceptanceMod<typename HMCWrapper::ImplPolicy> AccObs;
  TheHMC.Resources.template AddObservable<AccObs>(AccPar);

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
template <typename HMCWrapper>
DJ<HMCWrapper>::DJ(std::string deckName, int argc, char* argv[])
    : DJ(deckName, argc, argv, {}) {}

/* Acceptance Rate tuning */
template <typename HMCWrapper>
void DJ<HMCWrapper>::Tune() {
  // Set number of trajectories to tuning steps
  TheHMC.Parameters.Trajectories = AccPar.num_tuning_samples;

  // Define tuning and acceptance filenames
  std::string tuning_filename = "tuning.xml";
  std::string acceptance_filename = "acceptance.xml";

  if (std::filesystem::exists(tuning_filename)) {
    Grid::XmlReader reader(tuning_filename);

    // Read current tuning mode
    int tmp_buf;
    reader.readDefault("mode", tmp_buf);
    *AccPar.tuning_mode = static_cast<tuning_mode_t>(tmp_buf);

    // Load current MDsteps value
    reader.readDefault("MDsteps", TheHMC.Parameters.MD.MDsteps);
    // Load tuning counter
    reader.readDefault("tuning_ctr", AccPar.tuning_ctr);

    // Compare new traj with num_tuning_steps and cut off trajectories
    // If tuning is not init, then load the acceptance and traj
    if ((*AccPar.tuning_mode != tuning_mode_t::init) &&
        (std::filesystem::exists(acceptance_filename))) {
      Grid::XmlReader AccReader(acceptance_filename);
      AccReader.readDefault("acc", *AccPar.AcceptanceArray);
      AccReader.readDefault("traj", *AccPar.TrajectoryArray);

      int delta_traj =
          AccPar.TrajectoryArray->back() - TheHMC.Parameters.StartTrajectory;
      if (delta_traj > 0) {
        if (AccPar.TrajectoryArray->size() > delta_traj) {
          AccPar.TrajectoryArray->resize(AccPar.TrajectoryArray->size() -
                                         delta_traj);
          AccPar.AcceptanceArray->resize(AccPar.AcceptanceArray->size() -
                                         delta_traj);
          // Cut off extra steps
          TheHMC.Parameters.Trajectories -= AccPar.AcceptanceArray->size();
        } else {
          // If something has gone wrong and some intermediate information is
          // missing restart the tuning step (by doing nothging)
          std::cout << Grid::GridLogMessage
                    << "Lacking acceptance information between trajectories "
                    << TheHMC.Parameters.StartTrajectory << " and "
                    << AccPar.TrajectoryArray->front()
                    << " restarting tuning step." << std::endl;
        }
      } else if (delta_traj < 0) {
        std::cout << Grid::GridLogMessage << "Starting from trajectory "
                  << TheHMC.Parameters.StartTrajectory
                  << " but have tuning information up to "
                  << AccPar.TrajectoryArray->back()
                  << " restarting tuning step." << std::endl;
      }
    }

  } else {
    // First run; initialise tuning
    *AccPar.tuning_mode = tuning_mode_t::init;
    AccPar.tuning_ctr = 0;

    Grid::XmlWriter AccWriter(tuning_filename);
    write(AccWriter, "mode", static_cast<int>(*AccPar.tuning_mode));
    write(AccWriter, "tuning_ctr", 0);
    write(AccWriter, "MDsteps", TheHMC.Parameters.MD.MDsteps);
  }

  if (*AccPar.tuning_mode == tuning_mode_t::complete) {
    std::cout << Grid::GridLogMessage
              << "MDsteps has already been successfully tuned to: "
              << TheHMC.Parameters.MD.MDsteps << std::endl;
    return;
  }

  // Initialisation mode
  if (*AccPar.tuning_mode == tuning_mode_t::init) {
    // Set number of trajectories
    TheHMC.Parameters.Trajectories =
        reader.Thermalisations + AccPar.rethermalisation +
        AccPar.num_tuning_samples -
        TheHMC.Parameters.StartTrajectory;  // if restarting

    // Play once to thermalise and initialise the tuning process
    Play();

    // Activate tuning
    *AccPar.tuning_mode = tuning_mode_t::active;

    // Update HMC parameters
    TheHMC.Parameters.NoMetropolisUntil = 0;  // Deactivate no metropolis
    TheHMC.Parameters.StartTrajectory =       // Update starting traj
        TheHMC.Parameters.StartTrajectory + TheHMC.Parameters.Trajectories;
  }

  // Tuning loop
  while (*AccPar.tuning_mode != tuning_mode_t::complete) {
    // Run the HMC
    Play();

    // Update HMC parameters
    TheHMC.Parameters.StartTrajectory =  // Update starting traj
        TheHMC.Parameters.StartTrajectory + TheHMC.Parameters.Trajectories;

    // Increment tuning counter
    AccPar.tuning_ctr++;

    // Empty current trajectories
    AccPar.TrajectoryArray->clear();

    // Measure acceptance rate
    double pacc = mean(*AccPar.AcceptanceArray);
    double pacc_err = stdErr(*AccPar.AcceptanceArray);
    // Empty the acceptance array
    AccPar.AcceptanceArray->clear();

    std::cout << Grid::GridLogMessage << "Current acceptance rate is: " << pacc
              << " +/- " << pacc_err << std::endl;

    // Apply tuning if required
    if (fabs(pacc - AccPar.target_rate) < AccPar.target_rate_flex) {
      // Tuning complete
      *AccPar.tuning_mode = tuning_mode_t::complete;
      std::cout << Grid::GridLogMessage
                << "Tuning successful, with final MD steps: "
                << TheHMC.Parameters.MD.trajL << std::endl;

      // Save tuning final state
      Grid::XmlWriter AccWriter(tuning_filename);
      write(AccWriter, "mode", static_cast<int>(tuning_mode_t::complete));
      write(AccWriter, "tuning_ctr", AccPar.tuning_ctr);
      write(AccWriter, "MDsteps", TheHMC.Parameters.MD.MDsteps);

    } else {  // Bad acceptance rate - tune MD steps

      int target_MD = get_target_MDsteps(TheHMC.Parameters.MD.trajL,
                                         TheHMC.Parameters.MD.MDsteps, pacc,
                                         AccPar.target_rate);

      std::cout << Grid::GridLogMessage
                << "Best approximation for target MDsteps: " << target_MD
                << " with Dt: "
                << static_cast<double>(TheHMC.Parameters.MD.trajL) / target_MD
                << std::endl;

      TheHMC.Parameters.MD.MDsteps = target_MD;

      Grid::XmlWriter AccWriter(tuning_filename);
      write(AccWriter, "mode", static_cast<int>(*AccPar.tuning_mode));
      write(AccWriter, "tuning_ctr", AccPar.tuning_ctr);
      write(AccWriter, "MDsteps", TheHMC.Parameters.MD.MDsteps);
    }
  }
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
