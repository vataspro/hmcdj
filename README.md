Usage:
./bootstrap
mkdir build; cd build
../configure --with-grid=<path/to/grid> --prefix=<path/to/install/to> <any other configuration options, e.g. CXX>
make

Once the executable has been compiled, a test case is provided by the sample track.yaml provided in the home directory. In the build directory, run:

./hmcDJ ../track.yaml
