#pragma once

#include <Grid/Grid.h>
#include <hmcdj/utils/logging.h>
#include <hmcdj/utils/timing.h>

#include <iostream>
#include <sstream>
#include <string>

#ifndef HAVE_LIME
#error "LIME is required; please re-build Grid with LIME."
#endif

/* A checkpointer that also checks the time to trajectory completion,
   and can exit if it estimates insufficient time is available to complete
   another. Large portions of this have been borrowed from
   Grid/qcd/hmc/ILDGCheckpointer.h; reproduced because `Params` is declared as
   private, so it's impossible to override any methods that depend on it. */
template <class Implementation, int DJSuccessfulExit>
class ILDGTimingHmcCheckpointer
    : public Grid::BaseHmcCheckpointer<Implementation> {
 private:
  Grid::CheckpointerParameters Params;
  std::unique_ptr<TrajectoryTimer> timer;

 public:
  INHERIT_GIMPL_TYPES(Implementation);
  typedef Grid::GaugeStatistics<Implementation> GaugeStats;

  ILDGTimingHmcCheckpointer(const Grid::CheckpointerParameters &Params_) {
    initialize(Params_);
    timer = std::unique_ptr<TrajectoryTimer>(new TrajectoryTimer);
  };

  void initialize(const Grid::CheckpointerParameters &Params_) {
    Params = Params_;

    // check here that the format is valid
    int ieee32big = (Params.format == std::string("IEEE32BIG"));
    int ieee32 = (Params.format == std::string("IEEE32"));
    int ieee64big = (Params.format == std::string("IEEE64BIG"));
    int ieee64 = (Params.format == std::string("IEEE64"));

    if (!(ieee64big || ieee32 || ieee32big || ieee64)) {
      std::cout << DJLogError << "Unrecognized file format " << Params.format
                << std::endl;
      std::cout << DJLogError
                << "Allowed: IEEE32BIG | IEEE32 | IEEE64BIG | IEEE64"
                << std::endl;

      exit(1);
    }
  }

  /* On completing a trajectory,
     verify whether we have enough time for the next one,
     and quit if not.
     Checkpoint if quitting, or if required by checkpoint interval. */
  void TrajectoryComplete(int traj,
                          Grid::ConfigurationBase<GaugeField> &SmartConfig,
                          Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG) {
    TimerStatus status = timer->updateTiming(traj);
    if ((traj % Params.saveInterval == 0) || status != OK) {
      writeConfiguration(traj, SmartConfig, sRNG, pRNG);
    }

    if (status != OK) {
      haltAsOutOfTime(status);
    }
  };

 private:
  /* Tell the user why the process is exiting, and exit */
  void haltAsOutOfTime(const TimerStatus status) {
    std::cout << Grid::GridLogHMC
              << ":::::::::::::::::::::::::::::::::::::::::::" << std::endl;
    std::cout << DJLogTiming << "Stopping now by request of hmcdj."
              << std::endl;
    switch (status) {
      case PREEMPTED:
        std::cout << DJLogTiming << "Reason: Job has been pre-empted by Slurm."
                  << std::endl;
        break;
      case OUT_OF_TIME:
        std::cout << DJLogTiming
                  << "Reason: Insufficient time projected to complete another "
                     "trajectory."
                  << std::endl;
        break;
      default:
        std::cout << DJLogTiming << "Reason: Unknown (" << status << ")."
                  << std::endl;
        break;
    }
    Grid::Grid_finalize();
    std::exit(DJSuccessfulExit);
  };

  /* Write the given configuration to disk,
     constructing the filename.
     Borrowed from the body of Grid's ILDGHmcCheckpointer::TrajectoryComplete(),
     but omitting the check for trajectory index visibility.. */
  void writeConfiguration(int traj,
                          Grid::ConfigurationBase<GaugeField> &SmartConfig,
                          Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG) {
    std::string config, rng, smr;
    this->build_filenames(traj, Params, config, smr, rng);
    Grid::GridBase *grid = SmartConfig.get_U(false).Grid();
    uint32_t nersc_csum, scidac_csuma, scidac_csumb;
    Grid::BinaryIO::writeRNG(sRNG, pRNG, rng, 0, nersc_csum, scidac_csuma,
                             scidac_csumb);
    std::cout << DJLogMessage << "Written BINARY RNG " << rng << " checksum "
              << std::hex << nersc_csum << "/" << scidac_csuma << "/"
              << scidac_csumb << std::dec << std::endl;

    Grid::IldgWriter _IldgWriter(grid->IsBoss());
    _IldgWriter.open(config);
    _IldgWriter.writeConfiguration<GaugeStats>(SmartConfig.get_U(false), traj,
                                               config, config);
    _IldgWriter.close();

    std::cout << DJLogMessage << "Written ILDG Configuration on " << config
              << " checksum " << std::hex << nersc_csum << "/" << scidac_csuma
              << "/" << scidac_csumb << std::dec << std::endl;

    if (Params.saveSmeared) {
      Grid::IldgWriter _IldgWriter(grid->IsBoss());
      _IldgWriter.open(smr);
      _IldgWriter.writeConfiguration<GaugeStats>(SmartConfig.get_U(true), traj,
                                                 smr, smr);
      _IldgWriter.close();

      std::cout << DJLogMessage << "Written ILDG Configuration on " << smr
                << " checksum " << std::hex << nersc_csum << "/" << scidac_csuma
                << "/" << scidac_csumb << std::dec << std::endl;
    }
  };

  void CheckpointRestore(int traj, GaugeField &U, Grid::GridSerialRNG &sRNG,
                         Grid::GridParallelRNG &pRNG) {
    std::string config, rng, smr;
    this->build_filenames(traj, Params, config, smr, rng);
    this->check_filename(rng);
    this->check_filename(config);

    uint32_t nersc_csum, scidac_csuma, scidac_csumb;
    Grid::BinaryIO::readRNG(sRNG, pRNG, rng, 0, nersc_csum, scidac_csuma,
                            scidac_csumb);

    Grid::FieldMetaData header;
    Grid::IldgReader _IldgReader;
    _IldgReader.open(config);
    _IldgReader.readConfiguration<GaugeStats>(
        U, header);  // format from the header
    _IldgReader.close();

    std::cout << DJLogMessage << "Read ILDG Configuration from " << config
              << " checksum " << std::hex << nersc_csum << "/" << scidac_csuma
              << "/" << scidac_csumb << std::dec << std::endl;
  };
};

template <class ImplementationPolicy, int DJSuccessfulExit>
class ILDGTimingCPModule
    : public Grid::CheckPointerModule<ImplementationPolicy> {
  typedef Grid::CheckPointerModule<ImplementationPolicy> CPBase;
  using CPBase::CPBase;  // for constructors

  // acquire resource
  virtual void initialize() {
    this->CheckPointPtr.reset(
        new ILDGTimingHmcCheckpointer<ImplementationPolicy, DJSuccessfulExit>(
            this->Par_));
  };

 public:
  constexpr static const char *const Name = "hmcdj ILDG timing";
};
