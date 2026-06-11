#pragma once

#include <Grid/Grid.h>
#include <hmcdj/utils/logging.h>
#include <hmcdj/utils/timing.h>

#include <hmcdj/utils/acceptance.h>

#include <iostream>
#include <sstream>
#include <string>

#ifndef HAVE_LIME
#error "LIME is required; please re-build Grid with LIME."
#endif


// Special struct to handle the hard things in life
struct djCheckpointerParameters {
    Grid::CheckpointerParameters CheckpointerParams;
    AcceptanceObsParameters *AccParams = new AcceptanceObsParameters;
};

/* A checkpointer that also checks the time to trajectory completion,
   and can exit if it estimates insufficient time is available to complete
   another. Large portions of this have been borrowed from
   Grid/qcd/hmc/ILDGCheckpointer.h; reproduced because `Params` is declared as
   private, so it's impossible to override any methods that depend on it.

  DJSuccessfulExit should always be zero in production code.
   It should only be set to a non-zero value from a test harness,
   to allow detection of unwanted exits. */
template <class Implementation, int DJSuccessfulExit>
class ILDGTimingHmcCheckpointer
    : public Grid::BaseHmcCheckpointer<Implementation> {
 private:
//  Grid::CheckpointerParameters Params;
  djCheckpointerParameters Params;
  std::unique_ptr<TrajectoryTimer> timer;

 public:
  INHERIT_GIMPL_TYPES(Implementation);
  typedef Grid::GaugeStatistics<Implementation> GaugeStats;

  ILDGTimingHmcCheckpointer(const Grid::CheckpointerParameters &Params_) {
    initialize(Params_);
    timer = std::unique_ptr<TrajectoryTimer>(new TrajectoryTimer);
  };

  void initialize(const Grid::CheckpointerParameters &Params_) {
    Params.CheckpointerParams = Params_;

    // check here that the format is valid
    int ieee32big = (Params.CheckpointerParams.format == std::string("IEEE32BIG"));
    int ieee64big = (Params.CheckpointerParams.format == std::string("IEEE64BIG"));

    if (!(ieee64big || ieee32big)) {
      std::cout << DJLogError << "Unrecognized file format " << Params.CheckpointerParams.format
                << std::endl;
      std::cout << DJLogError
                << "Allowed: IEEE32BIG | IEEE64BIG"
                << std::endl;
      exit(1);
    }

    if ( !((Params.CheckpointerParams.group == std::string("su")) || (Params.CheckpointerParams.group == std::string("sp"))) ) {
      std::cout << Grid::GridLogError << "Unrecognized gauge group "
                                            << Params.CheckpointerParams.group << std::endl;
      std::cout << Grid::GridLogError << "Allowed: su | sp" << std::endl;
      exit(1);
    }

    if ( Params.CheckpointerParams.group == std::string("sp") && Grid::Nc%2!=0 ) {
      std::cout << Grid::GridLogError << "Nc=" << Grid::Nc;
      std::cout << ", Sp fields require even Nc" << std::endl;
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
    if ((traj % Params.CheckpointerParams.saveInterval == 0) || status != TimerStatus::OK) {
      writeConfiguration(traj, SmartConfig, sRNG, pRNG);
    }

    if (status != TimerStatus::OK) {
      haltAsOutOfTime(status);
    }
  };

  //overlad
  //
  // THIS IS THE ONE THAT'S CALLED
  void TrajectoryComplete(int traj,
                          Implementation::Field &U,
                          Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG) {
    TimerStatus status = timer->updateTiming(traj);
    if ((traj % Params.CheckpointerParams.saveInterval == 0) || status != TimerStatus::OK) {
      writeConfiguration(traj, U, sRNG, pRNG);
      saveAcceptance();
    }

    if (status != TimerStatus::OK) {
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
      case TimerStatus::PREEMPTED:
        std::cout << DJLogTiming << "Reason: Job has been pre-empted by Slurm."
                  << std::endl;
        break;
      case TimerStatus::OUT_OF_TIME:
        std::cout << DJLogTiming
                  << "Reason: Insufficient time projected to complete another "
                     "trajectory."
                  << std::endl;
        break;
      default:
        std::cout << DJLogTiming << "Reason: Unknown (" << as_integer(status)
                  << ")." << std::endl;
        break;
    }
    Grid::Grid_finalize();
    std::exit(DJSuccessfulExit);
  };

  /* choose appropriate template instantiation here since the
     non-const checkpointer parameters cannot be used directly as
     template arguments to IldgWriter */
  void chooseIldgWriter( std::string format, std::string group, bool reduced_matrix,
                         std::string lat_obj, int traj,
                         //Grid::ConfigurationBase<GaugeField> &SmartConfig,
                         Implementation::Field &U,
                         bool smeared) {

      Grid::GridBase *grid = U.Grid();//SmartConfig.get_U(smeared).Grid();


      Grid::IldgWriter _IldgWriter(grid->IsBoss());
      _IldgWriter.open(lat_obj);

      if(format=="IEEE64BIG") {
        if(group=="su" && reduced_matrix) {
          _IldgWriter.writeConfiguration<GaugeStats, Grid::GroupName::SU, Grid::MatrixFormat::REDUCED, Grid::FloatingPointFormat::IEEE64BIG>(U, traj, lat_obj, lat_obj);
        }
        else if (group=="su" && !reduced_matrix) {
          _IldgWriter.writeConfiguration<GaugeStats, Grid::GroupName::SU, Grid::MatrixFormat::FULL, Grid::FloatingPointFormat::IEEE64BIG>(U, traj, lat_obj, lat_obj);
        }
        else if (group=="sp" && reduced_matrix) {
          _IldgWriter.writeConfiguration<GaugeStats, Grid::GroupName::Sp, Grid::MatrixFormat::REDUCED, Grid::FloatingPointFormat::IEEE64BIG>(U, traj, lat_obj, lat_obj);
        }
        else if (group=="sp" && !reduced_matrix) {
          _IldgWriter.writeConfiguration<GaugeStats, Grid::GroupName::Sp, Grid::MatrixFormat::FULL, Grid::FloatingPointFormat::IEEE64BIG>(U, traj, lat_obj, lat_obj);
        }
      }
      else if (format=="IEEE32BIG") {
         if(group=="su" && reduced_matrix) {
          _IldgWriter.writeConfiguration<GaugeStats, Grid::GroupName::SU, Grid::MatrixFormat::REDUCED, Grid::FloatingPointFormat::IEEE32BIG>(U, traj, lat_obj, lat_obj);
        }
        else if (group=="su" && !reduced_matrix) {
          _IldgWriter.writeConfiguration<GaugeStats, Grid::GroupName::SU, Grid::MatrixFormat::FULL, Grid::FloatingPointFormat::IEEE32BIG>(U, traj, lat_obj, lat_obj);
        }
        else if (group=="sp" && reduced_matrix) {
          _IldgWriter.writeConfiguration<GaugeStats, Grid::GroupName::Sp, Grid::MatrixFormat::REDUCED, Grid::FloatingPointFormat::IEEE32BIG>(U, traj, lat_obj, lat_obj);
        }
        else if (group=="sp" && !reduced_matrix) {
          _IldgWriter.writeConfiguration<GaugeStats, Grid::GroupName::Sp, Grid::MatrixFormat::FULL, Grid::FloatingPointFormat::IEEE32BIG>(U, traj, lat_obj, lat_obj);
        }
      }

      _IldgWriter.close();
  }

// Overload
void chooseIldgWriter( std::string format, std::string group, bool reduced_matrix,
                         std::string lat_obj, int traj,
                         Grid::ConfigurationBase<GaugeField> &SmartConfig,
                         bool smeared) {
    chooseIldgWriter(format, group, reduced_matrix, lat_obj, traj, SmartConfig.get_U(smeared), smeared);
  }


  /* Write the given configuration to disk,
     constructing the filename.
     Borrowed from the body of Grid's ILDGHmcCheckpointer::TrajectoryComplete(),
     but omitting the check for trajectory index visibility.. */
  void writeConfiguration(int traj,
                          Grid::ConfigurationBase<GaugeField> &SmartConfig,
                          Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG) {
    std::string config, rng, smr;
    this->build_filenames(traj, Params.CheckpointerParams, config, smr, rng);
    uint32_t nersc_csum, scidac_csuma, scidac_csumb;
    Grid::BinaryIO::writeRNG(sRNG, pRNG, rng, 0, nersc_csum, scidac_csuma,
                             scidac_csumb);
    std::cout << DJLogMessage << "Written BINARY RNG " << rng << " checksum "
              << std::hex << nersc_csum << "/" << scidac_csuma << "/"
              << scidac_csumb << std::dec << std::endl;

    chooseIldgWriter(Params.CheckpointerParams.format, Params.CheckpointerParams.group, Params.CheckpointerParams.reduced_matrix, config, traj,
                      SmartConfig, false);

    std::cout << DJLogMessage << "Written ILDG Configuration on " << config
              << " checksum " << std::hex << nersc_csum << "/" << scidac_csuma
              << "/" << scidac_csumb << std::dec << std::endl;

    if (Params.CheckpointerParams.saveSmeared) {

      chooseIldgWriter(Params.CheckpointerParams.format, Params.CheckpointerParams.group, Params.CheckpointerParams.reduced_matrix, smr, traj,
                      SmartConfig, true);

      std::cout << DJLogMessage << "Written ILDG Configuration on " << smr
                << " checksum " << std::hex << nersc_csum << "/" << scidac_csuma
                << "/" << scidac_csumb << std::dec << std::endl;
    }
  };

  // Field version
  void writeConfiguration(int traj,
                          Implementation::Field &U,
                          Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG) {
    std::string config, rng, smr;
    this->build_filenames(traj, Params.CheckpointerParams, config, smr, rng);
    uint32_t nersc_csum, scidac_csuma, scidac_csumb;
    Grid::BinaryIO::writeRNG(sRNG, pRNG, rng, 0, nersc_csum, scidac_csuma,
                             scidac_csumb);
    std::cout << DJLogMessage << "Written BINARY RNG " << rng << " checksum "
              << std::hex << nersc_csum << "/" << scidac_csuma << "/"
              << scidac_csumb << std::dec << std::endl;

    chooseIldgWriter(Params.CheckpointerParams.format, Params.CheckpointerParams.group, Params.CheckpointerParams.reduced_matrix, config, traj,
                      U, false);

    std::cout << DJLogMessage << "Written ILDG Configuration on " << config
              << " checksum " << std::hex << nersc_csum << "/" << scidac_csuma
              << "/" << scidac_csumb << std::dec << std::endl;

    if (Params.CheckpointerParams.saveSmeared) {

      std::cout << DJLogMessage << "CANNOT WRITE SMEARED CONFIG OF FIELD" << std::endl;
    }
  };

  void CheckpointRestore(int traj, GaugeField &U, Grid::GridSerialRNG &sRNG,
                         Grid::GridParallelRNG &pRNG) {
    std::string config, rng, smr;
    this->build_filenames(traj, Params.CheckpointerParams, config, smr, rng);
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
  }

 void saveAcceptance() {
     std::cout << Grid::GridLogMessage
              << "Saving acceptance with target rate " << Params.AccParams->target_rate << std::endl;
 }


};

/* Wrap the ILDGTimingHmcCheckpointer in a CheckpointerModule
   so that it can be registered in an HMC.
   This is directly patterned on the built-in CheckpointerModules in Grid,
   but additionally allowing templating on the exit code. */
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
