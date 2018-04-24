#!/bin/bash

## Wrapepr script to handle all Moses' tuning parameters 

set -e
trap '' HUP

usage() {
  #                              1               2               3              4         5
  echo "Usage: `basename $0` <working dir> <source devset> <target devset> <moses.ini> <threads>"
  echo "If you want it to play nice you'll have to do it yourself"
}

if [ $# -ne 5 ]; then
  usage; exit 1;
fi

WORK=$1
SSET=$2
TSET=$3
MOSE=$4
THRD=$5

echo " ]>>- TUNING SCRIPT"
echo
echo "  >>- Changing to $WORK"
cd $WORK
echo "  >>- Executing tuning..."
$MO/scripts/training/mert-moses.pl $SSET $TSET \
$MO/bin/moses $MOSE --mertdir $MO/bin/ --decoder-flags="-threads $THRD" --threads $THRD &> tuning.out

echo
echo "  >>- Tuning over"
