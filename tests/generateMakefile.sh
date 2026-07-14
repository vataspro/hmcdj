#!/bin/bash

cd "$(dirname "$0")"

# Write header
cat <<EOF > Makefile.am
# Build gtest from ./external/
gtest_dir = \$(top_srcdir)/external/googletest/googletest
gtest_build = \$(top_srcdir)/tests/gtest
gmock_dir = \$(top_srcdir)/external/googletest/googlemock
gmock_build = \$(top_srcdir)/tests/gmock

# Build the gtest library
noinst_LIBRARIES = libgtest.a libgmock.a
libgtest_a_SOURCES = \$(gtest_dir)/src/gtest-all.cc
libgmock_a_SOURCES = \$(gmock_dir)/src/gmock-all.cc

# Include gtest when compiling tests
AM_CPPFLAGS = -I\$(gtest_dir) -I\$(gtest_dir)/include -I\$(gmock_dir) -I\$(gmock_dir)/include
AM_CPPFLAGS += -I\$(top_srcdir)

AM_CPPFLAGS += -DTOP_SRCDIR=\"\$(top_srcdir)\"

# Will make these programs when using make check
check_PROGRAMS =
TESTS =

# Tests to generate

EOF

# Loop through all .cc and .cpp files in the current directory
for src in *_test.cc; do
    # Skip if no .cc or .cpp files are found
    [ -e "$src" ] || continue

    exe="${src%.*}"

    # Add the program name and sources
    echo "check_PROGRAMS += $exe" >> Makefile.am
    echo "${exe}_SOURCES = $src" >> Makefile.am
    echo "${exe}_LDADD = libgtest.a \$(top_builddir)/hmcdj/libtestutils.a \$(top_builddir)/hmcdj/libhmcdj.a" >> Makefile.am
    echo "TESTS += $exe" >> Makefile.am
    echo >> Makefile.am
done
