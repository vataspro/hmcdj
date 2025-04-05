#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <yaml-cpp/yaml.h> // yaml-cpp is required


/* Guard function called on initialisation */
void djGuard(int argc, char* argv[]);

/* Functions for seeding pseudoRandom Number Generators */
int seedRNG(const std::string& name);
std::string genSerialSeed();

/* Ensemble Reader */
class EnsembleReader{
  private:
    /* Lattice dimensions */
    int nt, nx, ny, nz; // Lattice dims
    std::string dimString; // Lattice dimensions string

  public:
    /* Checkpointing */
    int saveInterval;
    std::string config_prefix, 
                rng_prefix,
                format;
    /* Action parameters */
    double beta;
    /* HMC parameters */
    double trajL;
    int MDsteps, Thermalisations, Trajectories;
    std::string StartingType;
    
    // Constructor - reads and loads the parameters from the yaml file
    EnsembleReader(const std::string filename);

    // Get the number of points in a particular lattice dimension
    int dimLength(const int i);

    // Gets a char pointer to the dimension string
    const char* GetDimStringPointer();

};

#endif