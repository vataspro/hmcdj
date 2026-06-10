#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/mathutils.h>

// enum class to define mode (phase) of tuning
// TODO: implement one final mode, "verifying"
enum class tuning_mode_t { init, active, complete };

// Serializable class for Acceptance Rate Tuning
struct AcceptanceObsParameters : Grid::Serializable {
  GRID_SERIALIZABLE_CLASS_MEMBERS(AcceptanceObsParameters, int,
                                  rethermalisation, int, num_tuning_samples,
                                  double, target_rate, double, target_rate_tol,
                                  int, tuning_ctr, int, monitor_every);

  // Tuning mode
  tuning_mode_t *tuning_mode = new tuning_mode_t;
  // Acceptance array
  std::vector<int> *AcceptanceArray = new std::vector<int>;
  // Trajectory number array
  std::vector<int> *TrajectoryArray = new std::vector<int>;
  // File to save acceptance
  std::string acceptance_filename = "acceptance.xml";

  AcceptanceObsParameters(int rethermalisation_ = 10,
                          int num_tuning_samples_ = 50,
                          double target_rate_ = 0.8,
                          double target_rate_tol_ = 0.05, int tuning_ctr_ = 0,
                          int monitor_every_ = 100)
      : rethermalisation(rethermalisation_),
        num_tuning_samples(num_tuning_samples_),
        target_rate(target_rate_),
        target_rate_tol(target_rate_tol_),
        tuning_ctr(tuning_ctr_),
        monitor_every(monitor_every_) {}
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
    // Save the acceptance and trajectory index
    if (*Pars.tuning_mode != tuning_mode_t::init) {
      // Print acceptance
      std::cout << Grid::GridLogDebug << "Step acceptance: [ " << traj << " ] "
                << static_cast<int>(accept) << std::endl;
      Pars.AcceptanceArray->push_back(static_cast<int>(accept));
      Pars.TrajectoryArray->push_back(static_cast<int>(traj));

      // Write acceptance and trajectory to file
      Grid::XmlWriter AccWriter(Pars.acceptance_filename);
      write(AccWriter, "acc", Pars.AcceptanceArray);
      write(AccWriter, "traj", Pars.TrajectoryArray);
    }

    // Monitor the acceptance rate
    if (*Pars.tuning_mode == tuning_mode_t::complete) {
      if (traj % Pars.monitor_every == 0) {
        double pacc = mean(*Pars.AcceptanceArray);
        std::cout << Grid::GridLogMessage
                  << "Monitoring current acceptance rate: " << pacc
                  << std::endl;

        if (fabs(pacc - Pars.target_rate) >= Pars.target_rate_tol) {
          std::cout << Grid::GridLogMessage
                    << "WARNING: Acceptance rate out of bounds";
        }
      }
    }
  }

  // SmartConfig version
  void TrajectoryComplete(int traj, Grid::ConfigurationBase<Field> &SmartConfig,
                          Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG, bool accept) override {
    TrajectoryComplete(traj, SmartConfig.get_U(false), sRNG, pRNG, accept);
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
