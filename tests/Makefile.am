# Build gtest from ./external/
gtest_dir = $(top_srcdir)/external/googletest/googletest
gtest_build = $(top_srcdir)/tests/gtest

# Build the gtest library
noinst_LIBRARIES = libgtest.a
libgtest_a_SOURCES = $(gtest_dir)/src/gtest-all.cc

# Include gtest when compiling tests
AM_CPPFLAGS = -I$(gtest_dir) -I$(gtest_dir)/include
AM_CPPFLAGS += -I$(top_srcdir)/hmcdj/utils
AM_CPPFLAGS += -DTOP_SRCDIR=\"$(top_srcdir)\"

# Will make these programs when using make check
check_PROGRAMS = run_test
run_test_SOURCES = run_test.cpp
run_test_LDADD = libgtest.a $(top_builddir)/hmcdj/libtestutils.a

TESTS = run_test
