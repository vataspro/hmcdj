#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/parameter.h>
#include <hmcdj/utils/utils.h>

#include "acceptance.h"
#include "observable_logging.h"

// Very advanced function
template <typename T>
double arrMean(std::vector<T> x) {
  T sum = 0;
  for (T el : x) {
    sum += el;
  }
  return static_cast<double>(sum) / x.size();
}

// Standard deviation
template <typename T>
double arrStdErr(std::vector<T> x) {
  double mean = arrMean(x);
  double sum = 0;
  for (T el : x) {
    sum += pow(el - mean, 2);
  }

  return sqrt(sum / (x.size() - 1) / x.size());
}

/*
 *  https://stackoverflow.com/questions/27229371/inverse-error-function-in-c
 *  Based on "A handy approximation of the error function and its inverse"
 *  by Sergei Winitzki
 */
inline float myErfInv(float x) {
  float tt1, tt2, lnx, sgn;
  sgn = (x < 0) ? -1.0f : 1.0f;

  x = (1 - x) * (1 + x);  // x = 1 - x*x;
  lnx = logf(x);

  tt1 = 2 / (M_PI * 0.147) + 0.5f * lnx;
  tt2 = 1 / (0.147) * lnx;

  return (sgn * sqrtf(-tt1 + sqrtf(tt1 * tt1 - tt2)));
}

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

  /* GRID PURE GAUGE STARTS HERE */

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
  AccPar.num_init_skip =
      10 + reader.Thermalisations;  // Thermalisation + initial skip
  AccPar.num_tuning_samples = 50;   // Number of samples to tune for
  AccPar.target_rate = 0.8;         // Target acceptance rate
  AccPar.target_rate_flex = 0.05;   // Flexibiility of acceptance rate
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
  // Create tuning file (or read it if it already exists)
  // For the first time only, skip the num_init_skips
  TheHMC.Parameters.Trajectories =
      AccPar.num_init_skip + AccPar.num_tuning_samples;
  // Run
  while (AccPar.tuning_active) {
    Play();
    // Tune it
    double mean = arrMean(*AccPar.AcceptanceArray);
    double stddev = arrStdErr(*AccPar.AcceptanceArray);

    std::cout << "ACCRATE IS: " << mean << " +/- " << stddev << std::endl;

    if (abs(mean - AccPar.target_rate) < AccPar.target_rate_flex) {
      // Tuning complete
      AccPar.tuning_active = false;
      std::cout << Grid::GridLogMessage << "Tuning complete!" << std::endl;
    } else {
      // To get the new acceptance rate we use the numerical formula
      // Δτ = 2 / sqrt(λ) * inverfc(pacc)
      double DeltaTau = static_cast<double>(TheHMC.Parameters.MD.trajL) /
                        TheHMC.Parameters.MD.MDsteps;
      // double lam = 4 * myErfInv2(mean) / pow(DeltaTau, 4);
      double DeltaTau_target =
          pow(pow(DeltaTau, 2) * myErfInv(AccPar.target_rate / mean), 0.5);
      std::cout << Grid::GridLogMessage << "Target Dt: " << DeltaTau_target
                << std::endl;

      int target_MD = static_cast<int>(std::round(
          static_cast<double>(TheHMC.Parameters.MD.trajL) / DeltaTau_target));
      std::cout << "Best I can do is MDsteps: " << target_MD << "with Dt: "
                << static_cast<double>(TheHMC.Parameters.MD.trajL) / target_MD
                << std::endl;

      TheHMC.Parameters.MD.MDsteps = target_MD;
    }

    TheHMC.Parameters.StartTrajectory =
        TheHMC.Parameters.StartTrajectory + TheHMC.Parameters.Trajectories;
    // AccPar.tuning_active = false;
  }
  // Tune : TheHMC.Resources.MDsteps += x
  // Update parameters;
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
