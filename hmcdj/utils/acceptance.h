#pragma once
#include <Grid/Grid.h>
#include <hmcdj/utils/mathutils.h>

// enum class to define mode (phase) of tuning
enum class tuning_mode_t { init, active, complete };

// Serializable class for Acceptance Rate Tuning
struct AcceptanceObsParameters : Grid::Serializable {
  GRID_SERIALIZABLE_CLASS_MEMBERS(AcceptanceObsParameters, int,
                                  total_num_init_skips, int, num_tuning_samples,
                                  double, target_rate, double, target_rate_tol,
                                  int, monitor_every);

  // Tuning mode
  tuning_mode_t *tuning_mode = new tuning_mode_t;
  // Acceptance array
  std::vector<int> *AcceptanceArray = new std::vector<int>;
  // File to save acceptance
  std::string acceptance_filename = "acceptance.xml";
  // File to save tuning state
  std::string tuning_filename = "tuning.xml";
  // Tuning counter
  int *tuning_ctr = new int;
  // Pointer to MDsteps
  unsigned int *MDsteps;
  // Flag for tuning
  bool *AcceptanceTuningActive = new bool;

  AcceptanceObsParameters(int total_num_init_skips_ = 10,
                          int num_tuning_samples_ = 50,
                          double target_rate_ = 0.8,
                          double target_rate_tol_ = 0.05, int tuning_ctr_ = 0,
                          int monitor_every_ = 100)
      : total_num_init_skips(total_num_init_skips_),
        num_tuning_samples(num_tuning_samples_),
        target_rate(target_rate_),
        target_rate_tol(target_rate_tol_),
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
    std::cout << Grid::GridLogDebug
              << "Tuning mode: " << static_cast<int>(*Pars.tuning_mode)
              << std::endl;

    // Save the acceptance and trajectory index
    if (*Pars.tuning_mode != tuning_mode_t::init) {
      // Print acceptance
      std::cout << Grid::GridLogDebug << "Step acceptance: [ " << traj << " ] "
                << static_cast<int>(accept) << std::endl;

      // Append the acceptance values
      Pars.AcceptanceArray->push_back(static_cast<int>(accept));
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
