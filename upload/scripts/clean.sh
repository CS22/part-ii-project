#!/bin/bash

## Wrapper script for Moses' corpus cleaning tool, taking a truecased corpus
## and outputting one with sentences violating the min/max length constraints
## removed.

set -e

echo "}} CLEANING SCRIPT {{"

usage() {
  echo "Usage: `basename $0` <dir/stem> <s-lang> <t-lang> <min> <max>"
  exit 1
}

ARGC=5

if [ $# -ne $ARGC ]; then
  usage;
fi

CORPUS=$1
S=$2
T=$3
MIN=$4
MAX=$5

$PJ/mosesdecoder/scripts/training/clean-corpus-n.perl \
  ${CORPUS}.true $S $T \
  ${CORPUS}.clean $MIN $MAX

echo "}} CLEANING SCRIPT COMPLETED {{"
