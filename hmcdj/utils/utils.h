#pragma once

#include <hmcdj/utils/ensemble.h>
#include <hmcdj/utils/parameter.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <yaml-cpp/yaml.h>

#include <Grid/sitmo_rng/sitmo_prng_engine.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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
