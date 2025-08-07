#pragma once

#include <Grid/Grid.h>
#include <hmcdj/utils/logging.h>
#include <signal.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <map>

enum TimerStatus { OK, PREEMPTED, OUT_OF_TIME };

class TrajectoryTimer {
 public:
  std::chrono::high_resolution_clock::time_point startTime, lastUpdate;

  // map from trajectory index to time taken to complete that trajectory
  std::map<int, std::chrono::high_resolution_clock::duration>
      trajectoryDurations;

  // Static as this must be set from a signal handler
  inline static bool preemptedAndStopping;

  // Save interval; required to be able to compute time until next save
  int saveInterval = 1;

  bool haveSchedulerDeadline;
  std::chrono::system_clock::time_point schedulerDeadline;

  TrajectoryTimer() {
    startTime = std::chrono::high_resolution_clock::now();
    lastUpdate = startTime;
    getSchedulerDeadline();
    setUpSignals();
    TrajectoryTimer::preemptedAndStopping = false;
  };

  TimerStatus updateTiming(const int traj) {
    // Ensure that we don't accidentally handle timing twice.
    assert(!trajectoryDurations.contains(traj));

    const std::chrono::high_resolution_clock::time_point currentTime =
        std::chrono::high_resolution_clock::now();
    trajectoryDurations[traj] = currentTime - lastUpdate;
    lastUpdate = currentTime;

    notifyProgress(traj);

    if (preemptedAndStopping) {
      return PREEMPTED;
    }
    if (!sufficientTimeForNextTrajectory()) {
      return OUT_OF_TIME;
    }
    return OK;
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
          std::chrono::seconds(atol(slurmDeadline)));
      auto const localDeadline =
          std::chrono::current_zone()->to_local(schedulerDeadline);
      std::cout << DJLogTiming << "Slurm requests we end before "
                << std::format("{:%Y-%m-%d %X}", localDeadline) << std::endl;
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
