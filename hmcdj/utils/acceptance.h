#pragma once
#include <Grid/Grid.h>

// Serializable class for Acceptance Rate Tuning
struct AcceptanceObsParameters : Grid::Serializable {
  GRID_SERIALIZABLE_CLASS_MEMBERS(AcceptanceObsParameters, int, aNumber)

  AcceptanceObsParameters(int anumber = 12) : aNumber(anumber) {}
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

  std::vector<double> log;

  // destructor
  virtual ~AcceptanceLogger() = default;

  // Get the acceptance of the step
  void TrajectoryComplete(int traj, Field &U, Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG, bool accept) override {
    // Placeholder -- print acceptance
    std::cout << Grid::GridLogMessage
              << "Acceptance: " << static_cast<int>(accept) << std::endl;
    log.push_back(static_cast<double>(accept));
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
