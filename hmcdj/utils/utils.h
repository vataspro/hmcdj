#pragma once

#include <openssl/evp.h>
#include <openssl/md5.h>
#include <yaml-cpp/yaml.h>

#include <Grid/sitmo_rng/sitmo_prng_engine.hpp>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

/* Guard function called on initialisation */
void djGuard(int argc, char* argv[]);

/* Functions for seeding pseudoRandom Number Generators */
uint32_t md5FileToInt(const std::string& filename);

/* sitmo Random Number Generator interface */
class RNGManager {
 private:
  sitmo::prng_engine engine;

 public:
  RNGManager(std::string filename);
  void Seed(std::string filename);
  std::string GenerateGridRNGSeedString();
  ~RNGManager() {};
};

/* Ensemble Reader */
class EnsembleReader {
 private:
  /* Lattice dimensions */
  int nt, nx, ny, nz;     // Lattice dims
  std::string dimString;  // Lattice dimensions string

 public:
  /* Checkpointing */
  int saveInterval;
  std::string config_prefix, rng_prefix, format;
  /* Action parameters */
  double beta;
  /* HMC parameters */
  double trajL;
  int MDsteps, Thermalisations, Trajectories;
  std::string StartingType;

  // Constructor - reads and loads the parameters from the yaml file
  EnsembleReader(const std::string filename);

  // Gets a char pointer to the dimension string
  const char* GetDimStringPointer();
};
