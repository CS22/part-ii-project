#!/bin/bash

## Script to translate a testset and compare it with a reference,
## using BLEU to score

set -e

echo "^^^ TRANSLATE and TEST script ^^^"

usage() {
  echo "Usage: `basename $0` <ini> <testset dir> <working dir> <stem -true> <s> <t> <corpus stem> <pivot dir [or: none]> [-test]"
  exit 1
}

ARGC=8
ARGM=9 #TWICE BELOW

if [ $# -lt $ARGC ]; then
  usage
elif [ $# = $ARGM ]; then
  if [ $9 = "-test" ]; then
    TEST=1
  else
    echo "ERROR! Malformed: $9"
    exit 2
  fi
elif [ $# -gt $ARGM ]; then
  usage
else
  TEST=0
fi

INI=$1
TSTDIR=$2
WORKING=$3
TSTSTM=$4
S=$5
T=$6
CORSTM=$7
PIVDIR=$8

if [ $PIVDIR = "none" ]; then
  SOURCE=$TSTDIR/${TSTSTM}.true.$S
  RESULT=$WORKING/${CORSTM}.${TSTSTM}.translated.$T
  SCORE=$WORKING/${CORSTM}.bleu-score.$TSTSTM.$S-$T
else
  SOURCE=$PIVDIR/${CORSTM}.${TSTSTM}.translated.$S
  RESULT=$WORKING/${CORSTM}.${TSTSTM}.pivot.translated.$T
  SCORE=$WORKING/pivot.${CORSTM}.bleu-score.$TSTSTM.$S-$T
fi

echo "^^^ TRANSLATING... ^^^"
nohup $MOSES -f $INI \
  < $SOURCE \
  > $RESULT \
 2> $WORKING/${CORSTM}.${TSTSTM}.out

if [ $TEST = 1 ]; then
  echo "^^^ SCORING... ^^^"
  $PJ/mosesdecoder/scripts/generic/multi-bleu.perl \
  -lc $TSTDIR/${TSTSTM}.true.$T \
  < $RESULT \
  > $SCORE
fi

echo "^^^ FINISHED! ^^^"
