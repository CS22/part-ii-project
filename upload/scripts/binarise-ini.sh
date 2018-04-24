#!/bin/bash

## Script to convert a Moses ini file to the required format for use with
## binarised phrase/reordering tables

if [ $# -ne 1 ]; then
  echo "Usage: `basename $0` <working dir>"
  exit 1;
fi

set -e

WORKING=$1
cp $WORKING/mert-work/moses.ini $WORKING/moses.ini

cat $WORKING/moses.ini\
|sed "s|PhraseDictionaryMemory|PhraseDictionaryCompact|"\
|sed "s|\(^Phra[^/]*$WORKING\)[^ ]*|\1/binarised-model/phrase-table.minphr|"\
|sed "s|\(^Lexi[^/]*$WORKING\)[^ ]*|\1/binarised-model/reordering-table|"\
> $WORKING/moses-compact.ini

rm $WORKING/moses.ini
