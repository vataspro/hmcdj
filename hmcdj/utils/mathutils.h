#pragma once
#include <Grid/Grid.h>

template <typename T>
double mean(std::vector<T> x) {
  T sum = 0;
  for (T el : x) {
    sum += el;
  }
  return static_cast<double>(sum) / x.size();
}

// Estimate probability from a finite sample:
template <typename T>
double clampedMean(std::vector<T> x) {
  const double mean_ = mean(x);
  if (mean_ == 0) {
    return 1.0 / x.size();
  }
  if (mean_ == 1) {
    return 1 - 1.0 / x.size();
  }
  return mean_;
}

// Standard deviation
template <typename T>
double stdErr(std::vector<T> x) {
  const double mean_ = mean(x);
  double sum = 0;
  for (T el : x) {
    sum += pow(el - mean_, 2);
  }
  return sqrt(sum / (x.size() - 1) / x.size());
}

// Uncertainty in estimate of mean of binary process from limited samples
template <typename T>
double stdErrProb(std::vector<T> x) {
  double mean_ = clampedMean(x);
  if (mean_ > 0.5) {
    mean_ = 1 - mean_;
  }
  return sqrt(mean_) * pow(x.size(), -0.5);
}

/*
 *  https://stackoverflow.com/questions/27229371/inverse-error-function-in-c
 *  Based on "A handy approximation of the error function and its inverse"
 *  by Sergei Winitzki
 */
inline float erfInv(float x) {
  float tt1, tt2, lnx, sgn;
  sgn = (x < 0) ? -1.0f : 1.0f;

  x = (1 - x) * (1 + x);  // x = 1 - x*x;
  lnx = logf(x);

  tt1 = 2 / (M_PI * 0.147) + 0.5f * lnx;
  tt2 = 1 / (0.147) * lnx;

  return (sgn * sqrtf(-tt1 + sqrtf(tt1 * tt1 - tt2)));
}

template <typename T>
inline double erfcinv(T x) {
  return static_cast<double>(erfInv(1 - static_cast<float>(x)));
}

/* Estimate the target number of MD steps
 * using the formula:
 *
 *   Δτ = 2 / λ * inverfc(pacc)
 *
 */
inline double get_target_MDsteps(double trajL, int MDsteps, double pacc,
                                 double target_pacc) {
  double dtau = trajL / MDsteps;
  double lam = 2 * erfcinv(pacc) / (dtau * dtau);
  double dtau_target = sqrt(2 * erfcinv(target_pacc) / lam);

  std::cout << Grid::GridLogDebug << "Estimated target dtau: " << dtau_target
            << std::endl;

  int target_MD =
      static_cast<int>(std::round(static_cast<double>(trajL) / dtau_target));

  // safety check
  if ((target_MD == 0) || (isnan(target_MD)) || (isinf(target_MD))) {
    std::cout << Grid::GridLogMessage << "MD steps best value is " << target_MD
              << ", terminating." << std::endl;
    exit(EXIT_FAILURE);
  }

  return target_MD;
}
