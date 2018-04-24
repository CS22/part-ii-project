#!/bin/bash

## Script for length cutting corora in all languages

set -e

echo ""

usage() {
  echo "Usage: `basename $0` <corpus size> <corpus dir> <test/dev size> <stem>"
  exit 1
}

ARGC=4

if [ $# -ne $ARGC ]; then
  usage
fi

CSIZE=$1
CORDIR=$2
TDSIZE=$3
STEM=$4

if ! [ -d corpus-$STEM ]; then mkdir corpus-$STEM; fi
if ! [ -d corpus-$STEM-devset ]; then mkdir corpus-$STEM-devset; fi
if ! [ -d corpus-$STEM-testset ]; then mkdir corpus-$STEM-testset; fi

for i in ar en es fr ru zh; do head -n $CSIZE $CORDIR/un.$i > corpus-$STEM/$STEM.$i; head -n $TDSIZE corpus-unpc-devset/UNv1.0.devset.$i > corpus-$STEM-devset/$STEM.devset.$i; \
head -n $TDSIZE corpus-unpc-testset/UNv1.0.testset.$i > corpus-$STEM-testset/$STEM.testset.$i; done

