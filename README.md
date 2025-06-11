# HMCDJ

**HMCDJ is a library that automates running Grid jobs
 through the use of `yaml` files.**

## Requirements

HMCDJ requires the [Grid](https://github.com/paboyle/Grid)
 and [yaml-cpp](https://github.com/jbeder/yaml-cpp) libraries.

## Compiling HMCDJ

After cloning the repository:

``` bash
Usage:
./bootstrap
mkdir build; cd build
../configure --with-grid=<path/to/grid> \
             --with-yaml-cpp=<path/to/yaml-cpp> \
             --prefix=<path/to/install/to> \
             <any other configuration options, e.g. CXX>
make
make -C decks
```

## Example usage

Once the executable has been compiled,
 a test case is provided by the sample
track.yaml provided in the home directory.
 In the build directory, run:

``` bash
./decks/PureGauge ../track.yaml
```
