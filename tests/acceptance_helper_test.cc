#include <gtest/gtest.h>
#include <hmcdj/utils/acceptance.h>

#include "run_test_helpers.h"

// Use prime numbers for all integer parameters
// to reduce the likelihood of tests accidentally passing
const int thermalisation = 2;
const int rethermalisation = 3;
const int tuningCycle = 7;
const int monitoringCycle = 11;
const int maxTuning = 103;
const int maxTrajectories = 301;
const double targetAcceptance = 0.85;
const double deltaTargetAcceptance = 0.05;
const int MDsteps = 23;
const double trajLength = M_PI;

// StepPoint is pretty simple, but worth testing regardless
TEST(StepPointTest, StepPointTest) {
  StepPoint sp(5, 10);
  EXPECT_EQ(sp.trajectoryIndex, 5);
  EXPECT_EQ(sp.stepCount, 10);
}

TEST(AcceptanceObsTest, CurrentTrajectoryTest) {
  AcceptanceObsParameters params(thermalisation, rethermalisation, tuningCycle,
                                 monitoringCycle, maxTuning, maxTrajectories,
                                 targetAcceptance, deltaTargetAcceptance);
  for (int trajectoryIndex = 0; trajectoryIndex < 2 * maxTuning;
       trajectoryIndex++) {
    EXPECT_EQ(params.currentTrajectory(), trajectoryIndex);
    params.acceptHistory->push_back(trajectoryIndex % 2);
  }
  EXPECT_EQ(params.currentTrajectory(), 2 * maxTuning);
}

TEST(AcceptanceObsTest, LastTuneIndexTest) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);
  EXPECT_EQ(params.lastTuneIndex(), thermalisation);
  params.MDsteps(10, 1);
  EXPECT_EQ(params.lastTuneIndex(), 10);
}

TEST(AcceptanceObsTest, MDStepsTest) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);
  EXPECT_EQ(params.MDsteps(), MDsteps);
  // Expect to have two copies of the initial MDsteps,
  // one at 0 and one at thermalisation
  EXPECT_EQ(params.stepCountHistory->size(), 2);
  // Test setting with an explicit trajectory index
  params.MDsteps(10, 1);
  EXPECT_EQ(params.MDsteps(), 1);

  // Test setting with an implicit trajectory index,
  // after fast-forwarding the run
  for (int i = 0; i < 20; i++) {
    params.acceptHistory->push_back(1);
  }
  params.MDsteps(2);
  EXPECT_EQ(params.MDsteps(), 2);
  EXPECT_EQ(params.stepCountHistory->size(), 4);
  EXPECT_EQ(params.stepCountHistory->back().trajectoryIndex, 20);
  EXPECT_EQ(params.stepCountHistory->back().stepCount, 2);
}

TEST(AcceptanceObsTest, TuningModeTest) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Start of run: initialising
  EXPECT_EQ(params.tuningMode(), tuning_mode_t::init);
  for (int i = 0; i < thermalisation; i++) {
    params.acceptHistory->push_back(1);
  }
  // Start of tuning: initialising
  for (int i = 0; i < rethermalisation; i++) {
    EXPECT_EQ(params.tuningMode(), tuning_mode_t::init);
    params.acceptHistory->push_back(1);
  }
  // Now into tuning
  for (int i = 0; i < tuningCycle * 2; i++) {
    EXPECT_EQ(params.tuningMode(), tuning_mode_t::active);
    params.acceptHistory->push_back(1);
  }
  // Re-tune puts us back into initialisation
  params.MDsteps(MDsteps + 1);
  for (int i = 0; i < rethermalisation; i++) {
    EXPECT_EQ(params.tuningMode(), tuning_mode_t::init);
    params.acceptHistory->push_back(1);
  }
  // Now back into tuning
  for (int i = params.currentTrajectory(); i < maxTuning; i++) {
    EXPECT_EQ(params.tuningMode(), tuning_mode_t::active);
    params.acceptHistory->push_back(1);
  }
  // Now monitoring
  for (int i = 0; i < maxTuning * 2; i++) {
    EXPECT_EQ(params.tuningMode(), tuning_mode_t::monitoring);
  }
}

TEST(AcceptanceObsTest, TuningModeFailedTest) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Advance the ensemble to near the maximum tune time
  for (int i = 0; i < maxTuning - tuningCycle + 1; i++) {
    params.acceptHistory->push_back(1);
  }
  // Reset MDsteps, simulating a re-tune very near the maximum tune time
  params.MDsteps(100);

  // The ensemble should now report that tuning has failed
  for (int i = 0; i < tuningCycle; i++) {
    EXPECT_EQ(params.tuningMode(), tuning_mode_t::failed);
    params.acceptHistory->push_back(1);
  }
}

TEST(AcceptanceObsTest, TrajectoriesToNextTuneTest) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Thermalisations don't count in trajectory count
  // (as they are requested from Grid via the NoMetropolisUntil parameter,
  // not the Trajectories parameter)
  for (int trajectories = 0; trajectories < thermalisation; trajectories++) {
    EXPECT_EQ(params.trajectoriesToNextTune(), rethermalisation + tuningCycle);
    params.acceptHistory->push_back(1);
  }
  // First cycle
  for (int trajectories = 0; trajectories < rethermalisation + tuningCycle;
       trajectories++) {
    EXPECT_EQ(params.trajectoriesToNextTune(),
              rethermalisation + tuningCycle - trajectories);
    params.acceptHistory->push_back(1);
  }
  // Subsequent cycles
  for (int trajectories = 0; trajectories < tuningCycle * 3; trajectories++) {
    EXPECT_EQ(params.trajectoriesToNextTune(),
              tuningCycle - trajectories % tuningCycle);
    params.acceptHistory->push_back(1);
  }
  // Tuning MDsteps resets
  params.MDsteps(1);
  for (int trajectories = 0; trajectories < rethermalisation + tuningCycle;
       trajectories++) {
    EXPECT_EQ(params.trajectoriesToNextTune(),
              rethermalisation + tuningCycle - trajectories);
    params.acceptHistory->push_back(1);
  }
  // Check we can go up until end of tuning correctly
  while (params.currentTrajectory() + tuningCycle < maxTuning) {
    for (int trajectories = 0; trajectories < tuningCycle; trajectories++) {
      EXPECT_EQ(params.trajectoriesToNextTune(),
                tuningCycle - trajectories % tuningCycle);
      params.acceptHistory->push_back(1);
    }
  }
  for (int trajectories = 0; params.currentTrajectory() < maxTuning;
       trajectories++) {
    EXPECT_EQ(params.trajectoriesToNextTune(),
              maxTuning - params.currentTrajectory());
    params.acceptHistory->push_back(1);
  }
  // We should now be in monitoring
  for (int trajectories = 0;
       params.currentTrajectory() < maxTrajectories - monitoringCycle;
       trajectories++) {
    EXPECT_EQ(params.trajectoriesToNextTune(),
              monitoringCycle - trajectories % monitoringCycle);
    params.acceptHistory->push_back(1);
  }
  // Approaching the maximum trajectory count
  for (int trajectories = 0; trajectories < monitoringCycle; trajectories++) {
    EXPECT_EQ(params.trajectoriesToNextTune(),
              maxTrajectories - params.currentTrajectory());
    params.acceptHistory->push_back(1);
  }
  // Should be finished now
  for (int trajectories = 0; trajectories < 10; trajectories++) {
    EXPECT_EQ(params.trajectoriesToNextTune(), 0);
    params.acceptHistory->push_back(1);
  }
}

TEST(AcceptanceObsTest, TrajectoriesToNextTuneTrajectoryEndTest) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Thermalisations don't count in trajectory count
  // (as they are requested from Grid via the NoMetropolisUntil parameter,
  // not the Trajectories parameter)
  for (int trajectories = 0; trajectories < thermalisation; trajectories++) {
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_FALSE(params.atEndOfTuningCycle());
  }
  // First cycle
  for (int trajectories = 0; trajectories < rethermalisation + tuningCycle - 1;
       trajectories++) {
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_FALSE(params.atEndOfTuningCycle());
  }
  params.acceptHistory->push_back(1);
  std::cout << params.currentTrajectory() << std::endl;
  EXPECT_TRUE(params.atEndOfTuningCycle());
  // Subsequent cycles
  for (int cycles = 0; cycles < 3; cycles++) {
    for (int trajectories = 0; trajectories < tuningCycle - 1; trajectories++) {
      params.acceptHistory->push_back(1);
      std::cout << params.currentTrajectory() << std::endl;
      EXPECT_FALSE(params.atEndOfTuningCycle());
    }
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_TRUE(params.atEndOfTuningCycle());
  }
  // Tuning MDsteps resets
  params.MDsteps(1);
  for (int trajectories = 0; trajectories < rethermalisation + tuningCycle - 1;
       trajectories++) {
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_FALSE(params.atEndOfTuningCycle());
  }
  params.acceptHistory->push_back(1);
  std::cout << params.currentTrajectory() << std::endl;
  EXPECT_TRUE(params.atEndOfTuningCycle());
  // Check we can go up until end of tuning correctly
  while (params.currentTrajectory() + tuningCycle < maxTuning) {
    for (int trajectories = 0; trajectories < tuningCycle - 1; trajectories++) {
      params.acceptHistory->push_back(1);
      std::cout << params.currentTrajectory() << std::endl;
      EXPECT_FALSE(params.atEndOfTuningCycle());
    }
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_TRUE(params.atEndOfTuningCycle());
  }
  while (params.currentTrajectory() < maxTuning - 1) {
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_FALSE(params.atEndOfTuningCycle());
  }
  params.acceptHistory->push_back(1);
  std::cout << params.currentTrajectory() << std::endl;
  EXPECT_TRUE(params.atEndOfTuningCycle());
  // We should now be in monitoring
  while (params.currentTrajectory() < maxTrajectories - monitoringCycle * 2) {
    for (int trajectories = 0; trajectories < monitoringCycle - 1;
         trajectories++) {
      params.acceptHistory->push_back(1);
      std::cout << params.currentTrajectory() << std::endl;
      EXPECT_FALSE(params.atEndOfTuningCycle());
    }
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_TRUE(params.atEndOfTuningCycle());
  }
  // Approaching the maximum trajectory count
  for (int trajectories = 0; trajectories < monitoringCycle - 1;
       trajectories++) {
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_FALSE(params.atEndOfTuningCycle());
  }
  params.acceptHistory->push_back(1);
  std::cout << params.currentTrajectory() << std::endl;
  EXPECT_TRUE(params.atEndOfTuningCycle());
  while (params.currentTrajectory() < maxTrajectories - 1) {
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_FALSE(params.atEndOfTuningCycle());
  }
  // Should be finished now
  for (int trajectories = 0; trajectories < 10; trajectories++) {
    params.acceptHistory->push_back(1);
    std::cout << params.currentTrajectory() << std::endl;
    EXPECT_TRUE(params.atEndOfTuningCycle());
  }
}

TEST(AcceptanceObsTest, AvgAcceptance50Test) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Set up a run with 50% acceptance
  for (int trajectory = 0; trajectory < thermalisation + rethermalisation;
       trajectory++) {
    // Period until end of rethermalisation is not averaged
    params.acceptHistory->push_back(1);
  }
  for (int trajectory = 0; trajectory < 6 * tuningCycle; trajectory++) {
    params.acceptHistory->push_back(trajectory % 2);
  }
  NumberWithError<double> result = params.avgAcceptance();
  EXPECT_EQ(result.value, 0.5);
  EXPECT_NEAR(result.error, 0.1091, 1e-4);
}

TEST(AcceptanceObsTest, AvgAcceptance100TestNoClamp) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Set up a run with 50% acceptance
  for (int trajectory = 0; trajectory < thermalisation + rethermalisation;
       trajectory++) {
    // Period until end of rethermalisation is not averaged
    params.acceptHistory->push_back(0);
  }
  for (int trajectory = 0; trajectory < 6 * tuningCycle; trajectory++) {
    params.acceptHistory->push_back(1);
  }
  NumberWithError<double> result = params.avgAcceptance();
  EXPECT_EQ(result.value, 1.0);
  EXPECT_NEAR(result.error, 0.02381, 1e-4);
}

TEST(AcceptanceObsTest, AvgAcceptance100TestClamp) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Set up a run with 50% acceptance
  for (int trajectory = 0; trajectory < thermalisation + rethermalisation;
       trajectory++) {
    // Period until end of rethermalisation is not averaged
    params.acceptHistory->push_back(0);
  }
  for (int trajectory = 0; trajectory < 6 * tuningCycle; trajectory++) {
    params.acceptHistory->push_back(1);
  }
  NumberWithError<double> result = params.avgAcceptance(true);
  EXPECT_EQ(result.value, 1.0 - 1.0 / 42.0);
  EXPECT_NEAR(result.error, 0.02381, 1e-4);
}

TEST(AcceptanceObsTest, AvgAcceptance0TestNoClamp) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Set up a run with 50% acceptance
  for (int trajectory = 0; trajectory < thermalisation + rethermalisation;
       trajectory++) {
    // Period until end of rethermalisation is not averaged
    params.acceptHistory->push_back(1);
  }
  for (int trajectory = 0; trajectory < 6 * tuningCycle; trajectory++) {
    params.acceptHistory->push_back(0);
  }
  NumberWithError<double> result = params.avgAcceptance();
  EXPECT_EQ(result.value, 0.0);
  EXPECT_NEAR(result.error, 0.02381, 1e-4);
}

TEST(AcceptanceObsTest, AvgAcceptance0TestClamp) {
  Grid::IntegratorParameters MDparams(MDsteps, trajLength);
  AcceptanceObsParameters params(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &MDparams);

  // Set up a run with 50% acceptance
  for (int trajectory = 0; trajectory < thermalisation + rethermalisation;
       trajectory++) {
    // Period until end of rethermalisation is not averaged
    params.acceptHistory->push_back(1);
  }
  for (int trajectory = 0; trajectory < 6 * tuningCycle; trajectory++) {
    params.acceptHistory->push_back(0);
  }
  NumberWithError<double> result = params.avgAcceptance(true);
  EXPECT_EQ(result.value, 1.0 / 42.0);
  EXPECT_NEAR(result.error, 0.02381, 1e-4);
}

TEST(AcceptanceObsTest, SaveLoadHistoryTest) {
  TemporaryDirectory tmpDir("SaveLoadHistoryTest");
  Grid::IntegratorParameters saverMDparams(MDsteps, trajLength);
  AcceptanceObsParameters saverParams(
      thermalisation, rethermalisation, tuningCycle, monitoringCycle, maxTuning,
      maxTrajectories, targetAcceptance, deltaTargetAcceptance, &saverMDparams);
  saverParams.setOutputDirectory(tmpDir.getDirectoryPath());

  for (int trajectory = 0; trajectory < 300; trajectory++) {
    saverParams.acceptHistory->push_back(trajectory % 3 && trajectory % 5);
  }
  for (int tune = 2; tune < 5; tune++) {
    saverParams.MDsteps(tune * 50, tune * tune);
  }
  saverParams.saveHistory();

  Grid::IntegratorParameters loaderMDParams(MDsteps, trajLength);
  AcceptanceObsParameters loaderParams(thermalisation, rethermalisation,
                                       tuningCycle, monitoringCycle, maxTuning,
                                       maxTrajectories, targetAcceptance,
                                       deltaTargetAcceptance, &loaderMDParams);
  loaderParams.setOutputDirectory(tmpDir.getDirectoryPath());
  loaderParams.loadHistory();
  ASSERT_EQ(loaderParams.acceptHistory->size(), 300);
  ASSERT_EQ(loaderParams.stepCountHistory->size(), 5);
  for (int trajectory = 0; trajectory < 300; trajectory++) {
    EXPECT_EQ((*loaderParams.acceptHistory)[trajectory],
              (trajectory % 3 && trajectory % 5));
  }
  EXPECT_EQ(loaderParams.stepCountHistory->front().stepCount, MDsteps);
  EXPECT_EQ(loaderParams.stepCountHistory->front().trajectoryIndex, 0);
  for (int tune = 2; tune < 5; tune++) {
    EXPECT_EQ((*loaderParams.stepCountHistory)[tune].stepCount, tune * tune);
    EXPECT_EQ((*loaderParams.stepCountHistory)[tune].trajectoryIndex,
              tune * 50);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
