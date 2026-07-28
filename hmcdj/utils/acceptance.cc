#include <hmcdj/utils/acceptance.h>

#include <cassert>

std::map<tuning_mode_t, std::string> tuningModeDescription{
    {tuning_mode_t::init, "Initialising"},
    {tuning_mode_t::active, "Tuning"},
    {tuning_mode_t::monitoring, "Monitoring"},
    {tuning_mode_t::failed, "Failed"},
    {tuning_mode_t::complete, "Run complete"}};

int AcceptanceObsParameters::MDsteps() const {
  return stepCountHistory->back().stepCount;
}

void AcceptanceObsParameters::MDsteps(const int trajectoryIndex,
                                      const int stepCount) {
  stepCountHistory->push_back(StepPoint(trajectoryIndex, stepCount));
  MD->MDsteps = stepCount;
}

void AcceptanceObsParameters::MDsteps(const int stepCount) {
  MDsteps(currentTrajectory(), stepCount);
}

void AcceptanceObsParameters::setOutputDirectory(
    std::filesystem::path directory) {
  acceptanceFilename = (directory / "acceptance.xml").string();
}

NumberWithError<double> AcceptanceObsParameters::avgAcceptance(
    const bool clamp) const {
  int startTrajectory = lastTuneIndex() + rethermalisationTrajectories;
  if (lastTuneIndex() < thermalisationTrajectories) {
    startTrajectory += thermalisationTrajectories;
  }
  if (currentTrajectory() > maxTuningTrajectories) {
    startTrajectory = maxTuningTrajectories;
  }
  const int trajectoryCount = currentTrajectory() - startTrajectory;
  if (trajectoryCount <= 0) {
    return NumberWithError<double>(NAN, NAN);
  }

  const double acceptance = mean(std::vector(
      acceptHistory->begin() + startTrajectory, acceptHistory->end()));
  double clampedAcceptance = acceptance;
  if (acceptance == 0.0) {
    clampedAcceptance = 1.0 / trajectoryCount;
  }
  if (acceptance == 1.0) {
    clampedAcceptance = 1 - 1.0 / trajectoryCount;
  }

  const double error = sqrt(std::min(clampedAcceptance, 1 - clampedAcceptance) /
                            trajectoryCount);
  return NumberWithError<double>(clamp ? clampedAcceptance : acceptance, error);
}

tuning_mode_t AcceptanceObsParameters::tuningMode() const {
  if (currentTrajectory() >= totalTrajectories) {
    return tuning_mode_t::complete;
  }
  if (lastTuneIndex() + rethermalisationTrajectories + tuningCycleTrajectories >
      maxTuningTrajectories) {
    return tuning_mode_t::failed;
  }
  if (currentTrajectory() >= maxTuningTrajectories) {
    return tuning_mode_t::monitoring;
  }
  // Verify that time has not gone backwards
  assert(currentTrajectory() < thermalisationTrajectories ||
         currentTrajectory() >= lastTuneIndex());

  if (currentTrajectory() < lastTuneIndex() + rethermalisationTrajectories) {
    return tuning_mode_t::init;
  }
  return tuning_mode_t::active;
}

bool AcceptanceObsParameters::atEndOfTuningCycle() const {
  if (tuningMode() == tuning_mode_t::complete) {
    // Completed, no more trajectories
    return true;
  }
  if (currentTrajectory() == maxTuningTrajectories) {
    return true;
  }
  if (tuningMode() == tuning_mode_t::monitoring) {
    if ((currentTrajectory() - maxTuningTrajectories) %
            monitoringCycleTrajectories ==
        0) {
      return true;
    }
    return false;
  }
  if (currentTrajectory() < lastTuneIndex() + rethermalisationTrajectories +
                                tuningCycleTrajectories) {
    return false;
  }
  if ((currentTrajectory() - lastTuneIndex() - rethermalisationTrajectories) %
          tuningCycleTrajectories ==
      0) {
    return true;
  }
  return false;
}

int AcceptanceObsParameters::trajectoriesToNextTune() const {
  if (tuningMode() == tuning_mode_t::complete) {
    // Completed, no more trajectories
    return 0;
  }
  if (tuningMode() == tuning_mode_t::monitoring) {
    const int targetTrajectories =
        monitoringCycleTrajectories -
        (currentTrajectory() - maxTuningTrajectories) %
            monitoringCycleTrajectories;
    if (targetTrajectories + currentTrajectory() <= totalTrajectories) {
      return targetTrajectories;
    } else {
      return std::max(totalTrajectories - currentTrajectory(), 0);
    }
  }
  if (currentTrajectory() < thermalisationTrajectories) {
    // We will do the remaining thermalisations plus a single cycle
    return rethermalisationTrajectories + tuningCycleTrajectories;
  }
  if (currentTrajectory() < lastTuneIndex() + rethermalisationTrajectories) {
    // Complete the rethermalisation and do a single cycle
    return lastTuneIndex() + rethermalisationTrajectories +
           tuningCycleTrajectories - currentTrajectory();
  }
  // We are mid-cycle
  const int targetTrajectories =
      tuningCycleTrajectories -
      (currentTrajectory() - lastTuneIndex() - rethermalisationTrajectories) %
          tuningCycleTrajectories;
  if (targetTrajectories + currentTrajectory() > maxTuningTrajectories) {
    return maxTuningTrajectories - currentTrajectory();
  }
  return targetTrajectories;
}

int AcceptanceObsParameters::currentTrajectory() const {
  return acceptHistory->size();
}

int AcceptanceObsParameters::lastTuneIndex() const {
  return stepCountHistory->back().trajectoryIndex;
}

template <>
void Grid::XmlWriter::writeDefault(const std::string &s,
                                   const std::vector<StepPoint> &x) {
  push(s);
  for (auto &u : x) {
    push("elem");
    write("trajectoryIndex", u.trajectoryIndex);
    write("stepCount", u.stepCount);
    pop();
  }
  pop();
}

template <>
void Grid::XmlReader::readDefault(const std::string &s,
                                  std::vector<StepPoint> &output) {
  if (!push(s)) {
    std::cout << DJLogError << "XML: Cannot open node '" << s << "'"
              << std::endl;
    std::exit(EXIT_FAILURE);
  }
  int trajectoryIndex;
  int stepCount;

  output.clear();
  for (int i = 0; node_.child("elem");) {
    push("elem");
    read("trajectoryIndex", trajectoryIndex);
    read("stepCount", stepCount);
    output.push_back(StepPoint(trajectoryIndex, stepCount));
    node_.set_name("elem-done");
    pop();
  }
  pop();
}

void AcceptanceObsParameters::saveHistory() {
  std::cout << DJLogDebug << "Saving acceptance with target rate "
            << targetAcceptance << std::endl;

  Grid::XmlWriter accWriter(acceptanceFilename);
  accWriter.writeDefault("acceptHistory", *acceptHistory);
  accWriter.writeDefault("stepCountHistory", *stepCountHistory);
}

void AcceptanceObsParameters::loadHistory() {
  // Load acceptance
  Grid::XmlReader accReader(acceptanceFilename);
  accReader.readDefault("acceptHistory", *acceptHistory);
  accReader.readDefault("stepCountHistory", *stepCountHistory);
}
