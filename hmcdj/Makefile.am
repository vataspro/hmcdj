lib_LIBRARIES = libhmcdj.a
libhmcdj_a_SOURCES = utils/utils.cc utils/dj.cc utils/timing.cc utils/logging.cc utils/ensemble.cc utils/acceptance.cc
AM_CPPFLAGS = -I$(top_srcdir)

noinst_LIBRARIES =

if BUILD_TESTS
noinst_LIBRARIES += libtestutils.a
libtestutils_a_SOURCES = utils/utils.cc
endif
