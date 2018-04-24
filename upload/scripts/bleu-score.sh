#!/bin/bash

## Wrapper script for calling the BLEU scoring tool to
## evaluate the quality of a translation in comparison
## to the actual (target language) text.

set -e

usage() {
  echo "Usage: `basename $0` <corpus> <translated>"
  exit 1
}

ARGC=2

if [ $# -ne $ARGC ]; then
  usage;
fi

CORPUS=$1
TRANS=$2

$PJ/mosesdecoder/scripts/generic/multi-bleu.perl \
  -lc $CORPUS \
  < $TRANS
