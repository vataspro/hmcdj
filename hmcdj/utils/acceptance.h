#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/ensemble.h>
#include <hmcdj/utils/logging.h>
#include <hmcdj/utils/mathutils.h>

// enum class to define mode (phase) of tuning
enum class tuning_mode_t { init = 0, active, monitoring, failed, complete };
extern std::map<tuning_mode_t, std::string> tuningModeDescription;

class StepPoint {
 public:
  // The index of the trajectory from which the step count is used
  const int trajectoryIndex;

  // The step count used in this interval
  const int stepCount;

  StepPoint(const int trajectoryIndex, const int stepCount)
      : trajectoryIndex(trajectoryIndex), stepCount(stepCount) {}
};

// Serializable class for Acceptance Rate Tuning
struct AcceptanceObsParameters : Grid::Serializable {
  GRID_SERIALIZABLE_CLASS_MEMBERS(
      AcceptanceObsParameters, int, rethermalisationTrajectories, int,
      tuningCycleTrajectories, double, targetAcceptance, double,
      deltaTargetAcceptance, int, monitoringCycleTrajectories, int,
      maxTuningTrajectories, int, thermalisationTrajectories);

  // Tuning mode
  tuning_mode_t tuningMode() const;
  int trajectoriesToNextTune() const;
  // Acceptance array
  // Since Grid creates a copy instance when constructing the HMC,
  // we must use shared_ptrs rather than unique_ptrs here
  std::shared_ptr<std::vector<int>> acceptHistory =
      std::make_shared<std::vector<int>>();
  int currentTrajectory() const;
  // History of step size changes
  std::shared_ptr<std::vector<StepPoint>> stepCountHistory =
      std::make_shared<std::vector<StepPoint>>();
  int lastTuneIndex() const;
  // File to save acceptance
  std::string acceptanceFilename = "/dev/null";
  // Pointer to MDsteps
  int MDsteps() const;
  void MDsteps(const int trajectoryIndex, const int numSteps);
  void MDsteps(const int numSteps);

  // Pointer to integrator to be able to control MDsteps
  Grid::IntegratorParameters *MD;

  AcceptanceObsParameters(int thermalisationTrajectories = 0,
                          int rethermalisationTrajectories = 10,
                          int tuningCycleTrajectories = 50,
                          int monitoringCycleTrajectories = 100,
                          int maxTuningTrajectories = 500,
                          double targetAcceptance = 0.8,
                          double deltaTargetAcceptance = 0.05,
                          Grid::IntegratorParameters *MD = nullptr)
      : thermalisationTrajectories(thermalisationTrajectories),
        rethermalisationTrajectories(rethermalisationTrajectories),
        tuningCycleTrajectories(tuningCycleTrajectories),
        targetAcceptance(targetAcceptance),
        deltaTargetAcceptance(deltaTargetAcceptance),
        monitoringCycleTrajectories(monitoringCycleTrajectories),
        maxTuningTrajectories(maxTuningTrajectories),
        MD(MD) {
    if (MD != nullptr) {
      MDsteps(0, MD->MDsteps);
    }
  }

  AcceptanceObsParameters(EnsembleReader reader,
                          Grid::IntegratorParameters *MD = nullptr)
      : AcceptanceObsParameters(
            reader.Thermalisations, reader.rethermalisationTrajectories,
            reader.tuningCycleTrajectories, reader.monitoringCycleTrajectories,
            reader.maxTuningTrajectories, reader.targetAcceptance,
            reader.deltaTargetAcceptance, MD) {
    setOutputDirectory(reader.EnsembleDirectory);
    MDsteps(0, reader.initialMDsteps);
    if (reader.StartingTrajectory > 0) {
      loadHistory();
    }
  }

  // Serialisation
  void saveHistory();
  void loadHistory();

  NumberWithError<double> avgAcceptance(const bool clamp = false) const;
  void setOutputDirectory(std::filesystem::path directory);
};

// Acceptance Rate Observable logger
template <class Impl>
class AcceptanceLogger : public Grid::HmcObservable<typename Impl::Field> {
  AcceptanceObsParameters Pars;

 public:
  // here forces the Impl to be of gauge fields
  // if not the compiler will complain
  INHERIT_GIMPL_TYPES(Impl);

  // Constructor
  AcceptanceLogger(AcceptanceObsParameters P) : Pars(P) {}

  // necessary for HmcObservable compatibility
  typedef typename Impl::Field Field;

  // destructor
  virtual ~AcceptanceLogger() = default;

  // Get the acceptance of the step
  void TrajectoryComplete(int traj, Field &U, Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG, bool accept) override {
    std::cout << DJLogDebug
              << "Tuning mode: " << tuningModeDescription[Pars.tuningMode()]
              << std::endl;

    // Save the acceptance and trajectory index
    if (Pars.tuningMode() != tuning_mode_t::init) {
      // Print acceptance
      std::cout << DJLogDebug << "Step acceptance: [ " << traj << " ] "
                << static_cast<int>(accept) << std::endl;
    }
    // Append the acceptance values
    Pars.acceptHistory->push_back(static_cast<int>(accept));
    assert(traj == Pars.acceptHistory->size());
  }

  void TrajectoryComplete(int traj, Field &U, Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG) override {}
};

// Acceptance Rate Observable Module
template <class Impl>
class AcceptanceMod : public Grid::ObservableModule<AcceptanceLogger<Impl>,
                                                    AcceptanceObsParameters> {
  typedef Grid::ObservableModule<AcceptanceLogger<Impl>,
                                 AcceptanceObsParameters>
      ObsBase;
  using ObsBase::ObsBase;  // for constructors

  // acquire resource
  virtual void initialize() {
    this->ObservablePtr.reset(new AcceptanceLogger<Impl>(this->Par_));
  }

 public:
  AcceptanceMod(AcceptanceObsParameters Par) : ObsBase(Par) {}
};
