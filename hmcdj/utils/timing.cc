#include <hmcdj/utils/timing.h>

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::system_clock sysclock;

TrajectoryTimer::TrajectoryTimer() {
  startTime = hrclock::now();
  lastUpdate = startTime;
  getSchedulerDeadline();
  setUpSignals();
  TrajectoryTimer::preemptedAndStopping = false;
}

TimerStatus TrajectoryTimer::updateTiming(const int traj) {
  // Ensure that we don't accidentally handle timing twice.
  assert(!trajectoryDurations.contains(traj));

  const hrclock::time_point currentTime = hrclock::now();
  trajectoryDurations[traj] = currentTime - lastUpdate;
  lastUpdate = currentTime;

  notifyProgress(traj);

  if (preemptedAndStopping) {
    return TimerStatus::PREEMPTED;
  }
  if (!sufficientTimeForNextTrajectory()) {
    return TimerStatus::OUT_OF_TIME;
  }
  return TimerStatus::OK;
}

void TrajectoryTimer::setUpSignals() {
  struct sigaction signalHandler;
  signalHandler.sa_handler = [](int signal) {
    TrajectoryTimer::preemptedAndStopping = true;
  };
  sigemptyset(&signalHandler.sa_mask);
  signalHandler.sa_flags = 0;

  sigaction(SIGUSR1, &signalHandler, NULL);
}

void TrajectoryTimer::notifyProgress(const int traj) {
  const int meanOnly = 0;
  std::cout << DJLogTiming << "Trajectory " << traj << " ("
            << trajectoryDurations.size() << " of this run) completed in "
            << std::chrono::duration<double>(trajectoryDurations[traj]) << "."
            << std::endl;
  std::cout << DJLogTiming << "Mean trajectory time is "
            << std::chrono::duration<double>(projectNextTrajectory(meanOnly))
            << "." << std::endl;
  if (trajectoryDurations.size() > 0 && haveSchedulerDeadline) {
    const int projectedRemainingTrajectories = projectRemainingTrajectories();
    if (projectedRemainingTrajectories > 0) {
      std::cout << DJLogTiming << "Project generating "
                << projectedRemainingTrajectories
                << " more trajectories this session, " << "reaching trajectory "
                << traj + projectedRemainingTrajectories << "." << std::endl;
    }
  }
}

void TrajectoryTimer::getSchedulerDeadline() {
  haveSchedulerDeadline = false;

  // Slurm provides a Unix timestamp (seconds since the epoch) for the end
  // time for the job
  if (const char *slurmDeadline = std::getenv("SLURM_JOB_END_TIME")) {
    haveSchedulerDeadline = true;
    schedulerDeadline =
        sysclock::time_point(std::chrono::seconds(atol(slurmDeadline)));
    auto const localDeadline =
        std::chrono::current_zone()->to_local(schedulerDeadline);
    std::cout << DJLogTiming << "Slurm requests we end before "
              << std::format("{:%Y-%m-%d %X}", localDeadline) << std::endl;
  }
}

int TrajectoryTimer::projectRemainingTrajectories() {
  const int meanOnly = 0;
  const sysclock::duration estimatedTrajectoryTime =
      projectNextTrajectory(meanOnly);
  if (estimatedTrajectoryTime.count() == 0) {
    // Sentinel value to indicate no data available.
    return -1;
  } else {
    return timeRemaining() / projectNextTrajectory(meanOnly);
  }
}

sysclock::duration TrajectoryTimer::timeRemaining() {
  assert(haveSchedulerDeadline);
  return schedulerDeadline - sysclock::now();
}

bool TrajectoryTimer::sufficientTimeForNextTrajectory() {
  const int safetyFactor = 2;
  return !haveSchedulerDeadline ||
         (sysclock::now() + projectNextTrajectory(safetyFactor) <
          schedulerDeadline);
}

/* Sum the elements and squares of elements of a std::map from int to duration,
   skipping at most one specified key. */
std::tuple<hrclock::duration, long> sumElementsAndSquares(
    std::map<int, hrclock::duration> vector, int skip_key) {
  using namespace std::chrono_literals;
  hrclock::duration sum = 0ms;
  long sumSquareMS =
      0;  // C++ durations cannot be squared, so this must be a long

  for (auto const &[key, duration] : vector) {
    if (key != skip_key) {
      sum += duration;
      const long durationMS = duration / 1ms;
      sumSquareMS += durationMS * durationMS;
    }
  }

  return {sum, sumSquareMS};
}

/* Given the sum and sum square of a duration,
   and the number of elements in the sum,
   compute the standard deviation. */
hrclock::duration stdDevDuration(long sumDurationMS, long sumSquareDurationMS,
                                 long durationCount) {
  using namespace std::chrono_literals;
  const double stdDurationUS =
      std::sqrt((double)(sumSquareDurationMS - sumDurationMS * sumDurationMS) *
                (double)durationCount / (durationCount - 1)) *
      1000;
  return std::lround(stdDurationUS) * 1us;
}

hrclock::duration stdDevDuration(hrclock::duration sumDuration,
                                 long sumSquareDurationMS, long durationCount) {
  using namespace std::chrono_literals;
  return stdDevDuration(sumDuration / 1ms, sumSquareDurationMS, durationCount);
}

/* Compute the projected duration of the next trajectory,
   from the mean of those that have been completed so far.
   Add padding of safetyFactor times the standard deviation,
   where available. */
hrclock::duration TrajectoryTimer::projectNextTrajectory(
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
      const int firstTrajectory = trajectoryDurations.begin()->first;
      const long numDurations = trajectoryDurations.size() - 1;

      auto [sumDuration, sumSquareDurationMS] =
          sumElementsAndSquares(trajectoryDurations, firstTrajectory);
      return sumDuration / numDurations +
             safetyFactor *
                 stdDevDuration(sumDuration, sumSquareDurationMS, numDurations);
  }
}
