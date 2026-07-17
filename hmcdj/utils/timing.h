#pragma once

#include <Grid/Grid.h>
#include <hmcdj/utils/logging.h>
#include <signal.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <map>
#include <tuple>

enum class TimerStatus { OK, PREEMPTED, OUT_OF_TIME, TUNING_FAILED };

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::system_clock sysclock;

hrclock::duration meanDurations(std::map<int, hrclock::duration> vector,
                                int skip_key);
std::tuple<hrclock::duration, hrclock::duration> meanStdDevDurations(
    std::map<int, hrclock::duration> vector, int skip_key);

/* Be able to print enum class value directly;
   from
   https://stackoverflow.com/questions/11421432/how-can-i-output-the-value-of-an-enum-class-in-c11
 */
template <typename Enumeration>
auto constexpr as_integer(Enumeration const value) ->
    typename std::underlying_type<Enumeration>::type {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

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

  TimerStatus updateTiming(const int traj);

  TrajectoryTimer();

 private:
  void setUpSignals();
  void notifyProgress(const int traj);
  void getSchedulerDeadline();
  int projectRemainingTrajectories();
  std::chrono::system_clock::duration timeRemaining();
  bool sufficientTimeForNextTrajectory();
  std::chrono::high_resolution_clock::duration projectNextTrajectory(
      const int safetyFactor);
};
