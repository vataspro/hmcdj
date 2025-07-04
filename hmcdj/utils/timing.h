#pragma once

#include <Grid/Grid.h>
#include <hmcdj/utils/logging.h>
#include <signal.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <map>

template <class Impl>
class TrajectoryTimer : public Grid::HmcObservable<typename Impl::Field> {
 public:
  /* Grid boilerplate */
  INHERIT_GIMPL_TYPES(Impl);
  typedef typename Impl::Field Field;

  std::chrono::high_resolution_clock::time_point startTime, lastUpdate;
  std::map<int, std::chrono::high_resolution_clock::duration>
      trajectoryDurations;

  // Static as this must be set from a signal handler
  inline static bool preemptedAndStopping;

  bool haveSchedulerDeadline;
  std::chrono::system_clock::time_point schedulerDeadline;

  virtual void TrajectoryComplete(int traj,
                                  Grid::ConfigurationBase<Field> &SmartConfig,
                                  Grid::GridSerialRNG &sRNG,
                                  Grid::GridParallelRNG &pRNG) {
    checkTiming(traj);
  };

  void TrajectoryComplete(int traj, Field &U, Grid::GridSerialRNG &sRNG,
                          Grid::GridParallelRNG &pRNG) {
    checkTiming(traj);
  };

  TrajectoryTimer() {
    startTime = std::chrono::high_resolution_clock::now();
    lastUpdate = startTime;
    getSchedulerDeadline();
    setUpSignals();
    TrajectoryTimer::preemptedAndStopping = false;
  };

 private:
  void setUpSignals() {
    struct sigaction signalHandler;
    signalHandler.sa_handler = [](int signal) {
      TrajectoryTimer::preemptedAndStopping = true;
    };
    sigemptyset(&signalHandler.sa_mask);
    signalHandler.sa_flags = 0;

    sigaction(SIGUSR1, &signalHandler, NULL);
  };

  void checkTiming(const int traj) {
    // Ensure that we don't accidentally handle timing twice.
    assert(!trajectoryDurations.contains(traj));

    const std::chrono::high_resolution_clock::time_point currentTime =
        std::chrono::high_resolution_clock::now();
    trajectoryDurations[traj] = currentTime - lastUpdate;
    lastUpdate = currentTime;

    notifyProgress(traj);
    if (preemptedAndStopping) {
      haltAsOutOfTime("Job has been pre-empted by Slurm.");
    }
    if (!sufficientTimeForNextTrajectory()) {
      haltAsOutOfTime(
          "Insufficient time projected to complete another trajectory.");
    }
  };

  void notifyProgress(const int traj) {
    const int meanOnly = 0;
    std::cout << DJLogTiming << "Trajectory " << traj << " ("
              << trajectoryDurations.size() << " of this run) completed in "
              << trajectoryDurations[traj] << "." << std::endl;
    std::cout << DJLogTiming << "Mean trajectory time is "
              << projectNextTrajectory(meanOnly) << "." << std::endl;
    if (trajectoryDurations.size() > 0 && haveSchedulerDeadline) {
      const int projectedRemainingTrajectories = projectRemainingTrajectories();
      if (projectedRemainingTrajectories > 0) {
        std::cout << DJLogTiming << "Project generating "
                  << projectedRemainingTrajectories
                  << " more trajectories this session, "
                  << "reaching trajectory "
                  << traj + projectedRemainingTrajectories << "." << std::endl;
      }
    }
  };

  void getSchedulerDeadline() {
    haveSchedulerDeadline = false;

    // Slurm provides a Unix timestamp (seconds since the epoch) for the end
    // time for the job
    if (const char *slurmDeadline = std::getenv("SLURM_JOB_END_TIME")) {
      haveSchedulerDeadline = true;
      schedulerDeadline = std::chrono::system_clock::time_point(
          std::chrono::seconds(atoi(slurmDeadline)));
    }
  };

  int projectRemainingTrajectories() {
    const int meanOnly = 0;
    const std::chrono::system_clock::duration estimatedTrajectoryTime =
        projectNextTrajectory(meanOnly);
    if (estimatedTrajectoryTime.count() == 0) {
      // Sentinel value to indicate no data available.
      return -1;
    } else {
      return timeRemaining() / projectNextTrajectory(meanOnly);
    }
  };

  std::chrono::system_clock::duration timeRemaining() {
    assert(haveSchedulerDeadline);
    return schedulerDeadline - std::chrono::system_clock::now();
  };

  bool sufficientTimeForNextTrajectory() {
    const int safetyFactor = 2;
    return !haveSchedulerDeadline || (std::chrono::system_clock::now() +
                                          projectNextTrajectory(safetyFactor) <
                                      schedulerDeadline);
  };

  void haltAsOutOfTime(const std::string reason) {
    std::cout << Grid::GridLogHMC
              << ":::::::::::::::::::::::::::::::::::::::::::" << std::endl;
    std::cout << DJLogTiming << "Stopping now by request of hmcdj."
              << std::endl;
    std::cout << DJLogTiming << "Reason: " << reason << std::endl;
    Grid::Grid_finalize();
    std::exit(0);
  };

  /* Compute the projected duration of the next trajectory,
     from the mean of those that have been completed so far.
     Add padding of safetyFactor times the standard deviation,
     where available. */
  std::chrono::high_resolution_clock::duration projectNextTrajectory(
      const int safetyFactor) {
    switch (trajectoryDurations.size()) {
      case 0:
        // We want to try one trajectory at least
        return std::chrono::milliseconds(0);
      case 1:
        // If we have only completed one trajectory, we don't have that much to
        // go on
        return trajectoryDurations.begin()->second;
      case 2:
        // For 2 and above, skip the first trajectory, as it may include
        // initialisation effects. For 2 specifically, we still don't have
        // enough data to compute a standard deviation.
        return trajectoryDurations.rbegin()->second;
      default:
        // Compute the mean and standard deviation of all apart from the first
        // trajectory.
        using namespace std::chrono_literals;
        std::chrono::high_resolution_clock::duration sumDuration = 0ms;
        long sumSquareDuration = 0;
        const int firstTrajectory = trajectoryDurations.begin()->first;
        for (auto const &[trajectory, trajectoryDuration] :
             trajectoryDurations) {
          if (trajectory != firstTrajectory) {
            sumDuration += trajectoryDuration;

            // We need to make a roundtrip to long because we can't square a
            // duration.
            long trajectoryDurationNS = trajectoryDuration / 1.0ns;
            sumSquareDuration += trajectoryDurationNS * trajectoryDurationNS;
          }
        }
        const long numDurations = trajectoryDurations.size() - 1;
        const std::chrono::high_resolution_clock::duration meanDuration =
            sumDuration / numDurations;
        const long meanDurationNS = meanDuration / 1.0ns;
        const double meanSquareDurationNS = sumSquareDuration / numDurations;
        const double stdDuration = std::sqrt(
            ((double)meanSquareDurationNS - meanDurationNS * meanDurationNS) *
            (double)numDurations / (numDurations - 1));

        return std::lround(meanDurationNS + safetyFactor * stdDuration) * 1ns;
    }
  };
};

/* We are not creating an observable,
   but we inherit from ObservableModule
   so that we may hook a callback into the end of the HMC trajectory. */
template <class Impl>
class TimingMod
    : public Grid::ObservableModule<TrajectoryTimer<Impl>, Grid::NoParameters> {
  typedef Grid::ObservableModule<TrajectoryTimer<Impl>, Grid::NoParameters>
      ObsBase;
  using ObsBase::ObsBase;

  virtual void initialize() {
    this->ObservablePtr.reset(new TrajectoryTimer<Impl>());
  }

 public:
  TimingMod() : ObsBase(Grid::NoParameters()) {}
};
