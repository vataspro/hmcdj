# Build gtest from ./external/
gtest_dir = $(top_srcdir)/external/googletest/googletest
gtest_build = $(top_srcdir)/tests/gtest

# Build the gtest library
noinst_LIBRARIES = libgtest.a
libgtest_a_SOURCES = $(gtest_dir)/src/gtest-all.cc

# Include gtest when compiling tests
AM_CPPFLAGS = -I$(gtest_dir) -I$(gtest_dir)/include
AM_CPPFLAGS += -I$(top_srcdir)
AM_CPPFLAGS += -DTOP_SRCDIR=\"$(top_srcdir)\"

# Will make these programs when using make check
check_PROGRAMS = utils_test
utils_test_SOURCES = utils_test.cpp
utils_test_LDADD = libgtest.a $(top_builddir)/hmcdj/libtestutils.a

TESTS = utils_test
