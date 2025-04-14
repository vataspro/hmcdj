#!/bin/bash

cd "$(dirname "$0")"

# Start by defining the necessary variables
echo "bin_PROGRAMS =" > Makefile.am
echo "AM_CPPFLAGS = -I\$(top_srcdir)" >> Makefile.am
echo "LDADD = \$(top_builddir)/hmcdj/libhmcdj.a" >> Makefile.am

# Loop through all .cc and .cpp files in the current directory
for src in *.cc *.cpp; do
    # Skip if no .cc or .cpp files are found
    [ -e "$src" ] || continue

    exe="${src%.*}"
    
    # Add the program name and sources
    echo "bin_PROGRAMS += $exe" >> Makefile.am
    echo "${exe}_SOURCES = $src" >> Makefile.am
done
