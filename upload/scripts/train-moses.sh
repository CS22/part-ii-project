#!/bin/bash

## Wrapper script to handle all of Moses' training parameters 

set -e
trap '' HUP

usage() { #                             1             2          3             4       5      6       7      8
  echo -e "Usage: `basename $0` <working dir> <corpus dir> <corpus stem> <S-lang> <T-lang> <lm> <lm order> <cores>"
}

echo "> >> >>> TRAINING SCRIPT <<< << <"

ARGC=8

if [ $# -ne $ARGC ]; then
  usage; exit 1;
fi


WORK=$1; echo ">> Working directory: $WORK"
CORP=$2; echo ">> Corpus directory: $CORP"
STEM=$3; S=$4; T=$5; echo ">> Source: $STEM.$S"; echo ">> Target: $STEM.$T"
LM=$6; ORDER=$7; echo ">> LM of order $ORDER from file $LM"
CORES=$8

echo "> >> Changing to $WORK... << <";
cd $WORK;

echo "> >> Attempting training... << <"
# The 8 is the ID for KenLM, the language model we use
  $MO/scripts/training/train-model.perl -root-dir $WORK/train -corpus $CORP/$STEM.clean -f $S -e $T -alignment grow-diag-final-and -reordering msd-bidirectional-fe -lm 0:$ORDER:$LM:8 -external-bin-dir $MO/tools -mgiza -mgiza-cpus $CORES -cores $CORES >& training.out
