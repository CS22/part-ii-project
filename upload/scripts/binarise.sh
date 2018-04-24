#!/bin/bash

## Script to binarise a Moses translation model. Will require
## binarise-ini to be called on the ini file before the system
## can be used.

set -e 
trap '' HUP

usage() {
  echo "`basename $0` <working>"
}

echo "<>> BINARISE SCRIPT <<>"

if [ $# -ne 1 ]; then
  usage; exit 1;
fi

WORKING=$1

if ! [ -d $WORKING/binarised-model ]; then
  echo "<>> Making binarised dir:"
  mkdir $WORKING/binarised-model
fi

cd $WORKING
echo "<>> Processing phrase table..."
$PJ/mosesdecoder/bin/processPhraseTableMin \
  -in train/model/phrase-table.gz -nscores 4 \
 -out binarised-model/phrase-table
echo "<>> Processing lexical table..."
$PJ/mosesdecoder/bin/processLexicalTableMin \
  -in train/model/reordering-table.wbe-msd-bidirectional-fe.gz \
 -out binarised-model/reordering-table

echo "<>> Creating moses.ini file:"
$PJ/scripts/binarise-ini.sh $WORKING

echo "<>> BINARISE SCRIPT COMPLETE <<>"
