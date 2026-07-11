---
title: "Installing hmcdj"
---

## Install Grid

Since hmcdj builds on top of Grid,
the first step of getting hmcdj working is to install Grid.
Currently,
hmcdj depends on [the TELOS Collaboration's fork of Grid][grid-telos].

Grid may be built for CPU or GPU,
with or without MPI,
and most Grid options can be configured as needed.
(In particular,
`--enable-Nc` and `--enable-Sp` are likely to be needed.)
The following are required:

- `--prefix` must be set,
  and Grid must be `make install`ed following build,
  so that hmcdj can find and link against it.
- Any fermion representations needed for theories of interest must be built.
  `--enable-fermion-reps` is required to be able to run with higher representations.
  `--disable-fermion-instantiations` is allowed,
  but then hmcdj will only build correctly for pure gauge theories;
  others will fail to compile or link.
- LIME support must be include with `--with-lime`,
  as hmcdj uses this to write configurations in ILDG format.
- GMP and MPFR support must be enabled with `--with-gmp` and `--with-mpfr`,
  as the RHMC relies on these.

## Install other dependencies

In addition to Grid,
hmcdj also depends on the [yaml-cpp][yaml-cpp] library.
This may be installed from your package manager
(Homebrew on Mac,
Apt or Yum on Linux,
Spack on clusters on which you don't have root permissions),
or can be manually installed from [the development GitHub][yaml-cpp].

## Download and build hmcdj

hmcdj can be cloned using Git:

``` shellsession
git clone --recurse-submodules https://github.com/telos-collaboration/hmcdj
```

Similarly to Grid,
we need to bootstrap and create a build directory

``` shellsession
cd hmcdj
./bootstrap.sh
mkdir build
```

We're now ready to configure and build hmcdj

``` shellsession
cd build
../configure \
    --with-grid=${GRID_PREFIX} \
    --with-yaml-cpp=/usr \
    'CXXFLAGS=-std=c++20'
make -j
```

Note that explicitly enabling C++20 support in the compiler is currently required,
as otherwise the `CXXFLAGS` inherited from Grid will set the compiler to C++17 mode.
If and when Grid is updated to use C++20,
this can be removed.

With the hmcdj core library built,
we can now build the deck of interest;
for example:

``` shellsession
cd decks
make PureGauge
```

This deck will be statically linked and can be used directly;
`make install` is not needed.

[grid-telos]: <https://github.com/telos-collaboration/Grid>
[yaml-cpp]: <https://github.com/jbeder/yaml-cpp>
