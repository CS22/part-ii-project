#!/bin/bash

### Script to prepare parallel texts ready for ###
### system training                            ###

set -e
trap '' HUP

usage() {
  echo "Usage: `basename $0` <stem> <lang> <dir>"
  exit 1
}

ARGC=3

if [ $# -ne $ARGC ]; then
  usage
fi

NAME=$1;
LANG=$2;
IND=$3 
OUTD=$IND

TOK=1; TRU=1;

# TOKENISATION

if [ $LANG = "en" -o $LANG = "es" -o $LANG = "fr" -o $LANG = "ru" ]; then
  echo ">> Language: $LANG"
#  if [ $TOK -eq 1 ]; then
    echo ">> Tokenising..."
    $MO/scripts/tokenizer/tokenizer.perl -l $LANG \
      < $IND/$NAME.$LANG \
      > $OUTD/$NAME.tok.$LANG
    echo

    echo ">> Training truecaser... "
    $MO/scripts/recaser/train-truecaser.perl \
      --corpus $OUTD/$NAME.tok.$LANG \
      --model $OUTD/truecase-model.$LANG
    echo ">> Truecasing... "
    $MO/scripts/recaser/truecase.perl \
      --model $OUTD/truecase-model.$LANG \
      < $OUTD/$NAME.tok.$LANG \
      > $OUTD/$NAME.true.$LANG
    echo

elif [ $LANG = "zh" ]; then
    echo ">> Segmenting... "
    $PJ/libs/sseg/segment.sh ctb $IND/$NAME.$LANG UTF-8 0 > $OUTD/$NAME.tok.$LANG
  # No need to truecase Chinese
  mv $OUTD/$NAME.tok.$LANG \
     $OUTD/$NAME.true.$LANG

elif [ $LANG = "ar" ]; then 
    echo ">> Segmenting... "
    $PJ/scripts/arabic-segmenter.sh $IND/$NAME.$LANG $OUTD/$NAME.tok.$LANG 7
    echo
  # No need for truecasing Arabic
  mv $OUTD/$NAME.tok.$LANG \
     $OUTD/$NAME.true.$LANG

else
  echo ">> Unrecognised language option!"
  exit 4
fi

echo
echo ">> Corpus preparation complete << "
exit 0;
