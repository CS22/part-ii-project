#!/bin/bash

## Wrapper script to filter a translation model for a specific
## input to speed up translation (for that input only)

usage() {
  echo "Must be called in WORKING directory"
  echo "Usage: `basename $0` <dir> <name stem> <moses.ini> <testset dir>"
  exit 1
}

ARGCOUNT=4

if [ $# -ne $ARGCOUNT ]; then
  usage;
fi;

DIR=$1
NAME=$2
INI=$3
CORPUS=$4

echo "[o] - FILTERING - [o]"
$PJ/mosesdecoder/scripts/training/filter-model-given-input.pl \
   $DIR $INI $CORPUS/$NAME \
  -Binarizer $PJ/mosesdecoder/bin/processPhraseTableMin
echo "[o] - FINISHED! - [o]"
