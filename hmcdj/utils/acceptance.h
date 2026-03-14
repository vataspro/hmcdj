#pragma once
#include <Grid/Grid.h>

// Acceptance Rate Observable logger
template <class Impl>
class AcceptanceLogger : public Grid::HmcObservable<typename Impl::Field> {
 public:
  // here forces the Impl to be of gauge fields
  // if not the compiler will complain
  INHERIT_GIMPL_TYPES(Impl);

  // necessary for HmcObservable compatibility
  typedef typename Impl::Field Field;

  // destructor
  virtual ~AcceptanceLogger() = default;

  // Get the acceptance of the step
  void TrajectoryComplete(int traj, Field &U, Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG, bool accept) override {
    // Placeholder -- print acceptance
    std::cout << Grid::GridLogMessage
              << "Acceptance: " << static_cast<int>(accept) << std::endl;
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
                                                    Grid::NoParameters> {
  typedef Grid::ObservableModule<AcceptanceLogger<Impl>, Grid::NoParameters>
      ObsBase;
  using ObsBase::ObsBase;  // for constructors

  // acquire resource
  virtual void initialize() {
    this->ObservablePtr.reset(new AcceptanceLogger<Impl>());
  }

 public:
  AcceptanceMod() : ObsBase(Grid::NoParameters()) {}
};
