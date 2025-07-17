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
tracks provided in the `example_tracks` directory.
 To run the Pure Gauge deck from the
build directory:

``` bash
./decks/PureGauge ../example_tracks/PureGaugeTrack.yaml
```

Please note that for a chosen deck the only valid
top level namespaces in the track are either `Global` or
the `deckName`.

## Testing

Tests have been written in 
[googletest](https://github.com/google/googletest).
 In order to run the tests,
 first run the following in the root of the repository.

``` bash
git submodule update --init --recursive
```

Now the `--enable-tests=yes` flag can be passed 
during configuration. 
This will automatically run the tests.

Beware that tests should be run on a compute node
with GPUs when Grid has been compiled with GPU enabled.
The `LD_LIBRARY_PATH` should also be updated on runtime
to contain any dynamically linked libraries 
(such as `mpfr`).

Please note that for a chosen deck the only valid
top level namespaces in the track are either `Global` or
the `deckName`.

## Testing

Tests have been written in [googletest](https://github.com/google/googletest). In order to run them, please first run the following in the home directory:

``` bash
git submodule update --init --recursive
```

This will automatically run the tests, when executing `make check`.

Beware that tests should be run on a compute node
with GPUs when Grid has been compiled with GPU enabled.
The `LD_LIBRARY_PATH` should also be updated on runtime
to contain any dynamically linked libraries 
(such as `mpfr`).
