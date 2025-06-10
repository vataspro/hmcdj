AC_PREREQ([2.63])
AC_INIT([hmcDJ],[v0.1],[https://github.com/telos-collaboration/hmcdj],[hmcDJ])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR([hmcdj/hmcdj.h])
AC_CONFIG_MACRO_DIR([.buildutils/m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])


AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AM_PROG_AR
AC_LANG([C++])

AC_ARG_WITH([yaml-cpp],
            [AS_HELP_STRING([--with-yaml-cpp=<prefix>],
                            [try this for a non-standard install prefix of yaml-cpp])],
            [CXXFLAGS="$CXXFLAGS -I$with_yaml_cpp/include"]
            [LDFLAGS="$LDFLAGS -L$with_yaml_cpp/lib -lyaml-cpp"])

AC_ARG_WITH([grid],
    [AS_HELP_STRING([--with-grid=<prefix>],
    [try this for a non-standard install prefix of Grid])],
    [PATH="$with_grid/bin$PATH_SEPARATOR$PATH"]
    [CXXFLAGS="$CXXFLAGS -I$with_grid/include"]
    [LDFLAGS="$LDFLAGS -L$with_grid/lib"])

AC_CHECK_PROG([GRIDCONF],[grid-config],[yes])
if test x"$GRIDCONF" != x"yes" ; then
    AC_MSG_ERROR([grid-config not found])
fi
if test x"$CXX" == x ; then
    CXX="`grid-config --cxx`"
elif test "$CXX" != "`grid-config --cxx`" ; then
    GRID_CXX="`grid-config --cxx`"
    AC_MSG_WARN([CXX differs from that reported by grid-config: "$CXX" (application) vs "$GRID_CXX" (grid-config)])
fi
if test x"$CXXLD" == x ; then
    CXXLD="`grid-config --cxxld`"
elif test "$CXXLD" != "`grid-config --cxxld`" ; then
    GRID_CXXLD="`grid-config --cxxld`"
    AC_MSG_WARN([CXXLD differs from that reported by grid-config: "$CXXLD" (application) vs "$GRID_CXXLD" (grid-config)])
fi
CXXFLAGS="$CXXFLAGS `grid-config --cxxflags`"
CXXFLAGS="$AM_CXXFLAGS $CXXFLAGS"

LDFLAGS="$LDFLAGS `grid-config --ldflags`"
LDFLAGS="$AM_LDFLAGS $LDFLAGS"
LIBS=" -ldl -lGrid $LIBS `grid-config --libs`"

AC_MSG_CHECKING([that a minimal Grid program compiles]);
AC_LINK_IFELSE(
	[AC_LANG_SOURCE([[
    #include <Grid/Grid.h>

    using namespace Grid;

    int main(int argc, char *argv[])
    {
        Grid_init(&argc, &argv);
        Grid_finalize();

        return 0;
    }

    ]])],
	[AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])]
    [AC_MSG_ERROR([impossible to compile a minimal Grid program])])


AC_MSG_CHECKING([that yaml-cpp is available]);
AC_LINK_IFELSE(
	[AC_LANG_SOURCE([[
    #include <yaml-cpp/yaml.h>

    int main(int argc, char *argv[])
    {

        return 0;
    }

    ]])],
	[AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])]
    [AC_MSG_ERROR([yaml-cpp is not available])])

boop_CXX="$CXX"
boop_CXXLD="$CXXLD"
boop_CXXFLAGS="$CXXFLAGS"
boop_LDFLAGS="$LDFLAGS"
boop_LIBS="$LIBS"
boop_SHA=`git rev-parse HEAD`
boop_BRANCH=`git rev-parse --abbrev-ref HEAD`

AC_SUBST([CXXLD])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])
AC_SUBST([boop_CXX])
AC_SUBST([boop_CXXLD])
AC_SUBST([boop_CXXFLAGS])
AC_SUBST([boop_LDFLAGS])
AC_SUBST([boop_LIBS])
AC_SUBST([boop_SHA])
AC_SUBST([boop_BRANCH])

AC_CONFIG_FILES([
    Makefile
    hmcdj/Makefile
    decks/Makefile])
AC_OUTPUT
