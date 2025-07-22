lib_LIBRARIES = libhmcdj.a
libhmcdj_a_SOURCES = utils/utils.cc utils/dj.cc
AM_CPPFLAGS = -I$(top_srcdir)

noinst_LIBRARIES =

if BUILD_TESTS
noinst_LIBRARIES += libtestutils.a
libtestutils_a_SOURCES = utils/utils.cc
endif
