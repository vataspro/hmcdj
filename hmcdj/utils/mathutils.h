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

// Standard deviation
template <typename T>
double stdErr(std::vector<T> x) {
  double mean_ = mean(x);
  double sum = 0;
  for (T el : x) {
    sum += pow(el - mean_, 2);
  }

  return sqrt(sum / (x.size() - 1) / x.size());
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
