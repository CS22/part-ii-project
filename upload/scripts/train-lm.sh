#!/bin/bash

## Wrapper script to train and binarise a languge model

set -e

echo ")) LM TRAINING SCRIPT (("

usage() {
  echo "Usage: `basename $0` <lm dir> <order> <corpus dir> <stem (no true!)> <lmstem> <lang>"
  exit 1
}

ARGC=6

if [ $# -ne $ARGC ]; then
  usage;
fi

DIR=$1
ORDER=$2
CORPUS=$3
STEM=$4
LMSTEM=$5
LANG=$6

cd $DIR
echo ")) TRAINING... (("
$PJ/mosesdecoder/bin/lmplz -o $ORDER <$CORPUS/${STEM}.true.$LANG > ${LMSTEM}.arpa.$LANG

echo ")) BINARISING... (("
$PJ/mosesdecoder/bin/build_binary ${LMSTEM}.arpa.$LANG ${LMSTEM}.blm.$LANG
echo ")) FINISHED! (("
