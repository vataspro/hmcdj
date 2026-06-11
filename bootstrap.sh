#!/bin/bash

set -e

./decks/generateMakefile.sh
./tests/generateMakefile.sh
mkdir -p .buildutils/m4
autoreconf -fvi
