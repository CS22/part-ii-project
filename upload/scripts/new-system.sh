#!/bin/bash

### MASTER SCRIPT for system creation 

set -e
trap '' HUP

echo 
echo "==-->> ----------------- <<--=="
echo "==-->> NEW SYSTEM SCRIPT <<--=="
echo "==-->> ----------------- <<--=="
echo 

usage() {
  echo "Usage: `basename $0` ( new control file dir   |   control file )   [ cores ]"
  echo "Give a directory to create a skeleton control file,"
  echo "or give a control file name for creation/execution "
  exit 1
}

SCR="$PJ/scripts"
prepare-corpus() { $SCR/prepare-corpus.sh $@;}
clean() { $SCR/clean.sh $@;}
train-lm() { $SCR/train-lm.sh $@;}
train-moses() { $SCR/train-moses.sh $@;}
tune() { $SCR/tune-moses.sh $@;}
binarise() { $SCR/binarise.sh $@;}
filter() { $SCR/filter-model.sh $@;}
bleu() { $SCR/translate-for-bleu.sh $@;}

ARGC=1

if [ $# -eq 2 ]; then
  CC=$2
elif [ $# -ne $ARGC ]; then
  usage
fi

ARG=$1

newctrl() {
  echo "]] Creating new skeleton control file..."
  echo \
"Please use absolute paths.
If lm dir is already defined, a language model is expected.

corpus dir=$NPJ/corpus/unpc/
corpus stem=
source language=
target language=

devset dir=$NPJ/corpus/unpc/devset
devset stem=devset
testset dir=$NPJ/corpus/unpc/testset
testset stem=testset

lm dir=$NPJ/lm/unpc/
lm stem=
lm order=3

working dir=$NPJ/working/unpc/

min sentence length=1
max sentence length=80
cores (train)=8
thread number (tuning)=8

pivot dir=none
" > $1
}

inewctrl() {
# expects pN-bl.ss-tt
#      or pN.ss-tt

  cd $1
  SKEM=$(basename $PWD)
  PAIR=$(echo -n $SKEM | tail -c 5)
     S=$(echo -n $PAIR | head -c 2)
     T=$(echo -n $PAIR | tail -c 2)
    DS=$(cd $PWD/../devset; pwd)
    TS=$(cd $PWD/../testset; pwd)
   LTM=$(echo -n $SKEM | head -c -6)
   LMD=$(echo -n $PWD | sed 's/corpus/lm/')
    LM=$(dirname $LMD)/${LTM}.$T

   WRK=$(echo -n $PWD | sed 's/corpus/working/')

  if [ -z $CC ]; then
    CRS=8 # Default cores
  else
    CRS=$CC
  fi

  SEP=$(echo -n $SKEM | head -c 3 | tail -c 1) # `.` if pivot, `-` if baseline
  if   [ $SEP = "-" ]; then PIVDIR="none"      # but not pivot if it's -en!!
  elif [ $SEP = "." ]; then 
    if [ $T = "en" ]; then
      PIVDIR="none";
    else
      PIVDIR=$(echo -n "$PWD/../${LTM}.${T}-en" | sed 's/corpus/working/');
    fi
  else
    echo "ERROR! Seperator (e.g. [p0]. or [p0]-) was <$SEP>"; exit 1000
  fi

  echo "]] Attempting intelligent skeleton creation..."
  echo \
"Please use absolute paths.
If lm dir is already defined, a language model is expected.

corpus dir=$PWD
corpus stem=$SKEM
source language=$S
target language=$T

devset dir=$DS
devset stem=devset
testset dir=$TS
testset stem=testset

lm dir=$LM
lm stem=$LTM
lm order=3

working dir=$WRK

min sentence length=1
max sentence length=80
cores (train)=$CRS
thread number (tuning)=$CRS

pivot dir=$PIVDIR
" > ${SKEM}.ctrl
}

if [ -d $ARG ]; then
  echo "]] $ARG is a directory"
  inewctrl $ARG
  exit 42
elif ! [ -f $ARG ]; then
  echo "]] $ARG doesn't exits, creating..."
  newctrl $ARG
  exit 43
fi

if [ -f $ARG ]; then
  echo "]] $ARG is a file"
  echo "]] Processing control file..."
  s() {
    cat $ARG |sed -n "s/$1=\(.*\)/\1/p"
  }
  CORDIR=$(s "corpus dir"); 
  CORSTM=$(s "corpus stem"); 
       S=$(s "source language");
       T=$(s "target language");
  MINSEN=$(s "min sentence length");
  MAXSEN=$(s "max sentence length");
   LMDIR=$(s "lm dir");
   LMSTM=$(s "lm stem");
   LMORD=$(s "lm order");
  WRKDIR=$(s "working dir");
  TRNCRS=$(s "cores (train)");
  DEVSTM=$(s "devset stem");
  DEVDIR=$(s "devset dir");
  TUNTHR=$(s "thread number (tuning)");
  TSTSTM=$(s "testset stem");
  TSTDIR=$(s "testset dir");
  PIVDIR=$(s "pivot dir");
  echo "]] Control file processed."
  echo "]]"

  #Usage: prepare-corpus.sh <stem> <lang> <dir>
  DIRSTM=$CORDIR/$CORSTM
  prep() {
    # prep dir stem name
    if [ -f $1/$2.true.$S ]; then
      echo "]] SOURCE $3 truecased, skipping..."
    else
      echo "]] Preparing SOURCE $3..."
      echo "]]"
      prepare-corpus $2 $S $1
    fi
  
    if [ -f $1/$2.true.$T ]; then
      echo "]] TARGET $3 truecased, skipping..."
      echo "]]"
    else
      echo "]] Preparing TARGET $3..."
      echo "]]"
      prepare-corpus $2 $T $1 
    fi
  }
  prep $CORDIR $CORSTM "corpus"

  #Usage: clean.sh <dir/stem> <s-lang> <t-lang> <min> <max>
  if [ -f $DIRSTM.clean.$S ] && [ -f $DIRSTM.clean.$T ]; then
    echo "]] Corpus already CLEANED, skipping..."
    echo "]]"
  else
    echo "]] Cleaning corpus..."
    echo "]]"
    clean $DIRSTM $S $T $MINSEN $MAXSEN
  fi
 
  #Usage: train-lm.sh <lm dir> <order> <corpus dir> <stem (no true!)> <lmstem> <lang>
  BLM=$LMDIR/$LMSTM.blm.$T
  if [ -f $BLM  ]; then
    echo "]] LANGUAGE MODEL already trained, skipping..."
    echo "]]"
  else
    if ! [ -d $LMDIR ]; then 
      echo "]] Making dir `basename $LMDIR`..."
      mkdir $LMDIR
    fi
    echo "]] Training language model..."
    echo "]]"
    train-lm $LMDIR $LMORD $CORDIR $CORSTM $LMSTM $T
  fi
 
  #Usage: train-moses.sh <working dir> <corpus dir> <corpus stem> <S-lang> <T-lang> <lm> <lm order> <cores>
  if [ -f $WRKDIR/train/model/moses.ini ]; then
    echo "]] Moses already TRAINED, skipping..."
    echo "]]"
  else
    if ! [ -d $WRKDIR ]; then mkdir $WRKDIR; fi
    echo "]] Training moses..."
    echo "]]"
    train-moses $WRKDIR $CORDIR $CORSTM $S $T $BLM $LMORD $TRNCRS
  fi

  #Usage: tune-moses.sh <working dir> <source devset> <target devset> <moses.ini> <threads>
  if [ -f $WRKDIR/mert-work/moses.ini ]; then
    echo "]] Moses already TUNED, skipping..."
    echo "]]"
  else 
    # prep dir stem name
    prep $DEVDIR $DEVSTM "devset"
    echo "]] Tuning moses..."
    echo "]]"
    tune $WRKDIR $DEVDIR/$DEVSTM.true.$S $DEVDIR/$DEVSTM.true.$T $WRKDIR/train/model/moses.ini $TUNTHR
  fi


  #binarise.sh <working>
  if [ -f $WRKDIR/moses-compact.ini ]; then
    echo "]] Model already binarised, skipping..."
    echo "]]"
  else
    echo "]] BINARISING moses..."
    echo "]]"
    binarise $WRKDIR
  fi


  #Must be called in WORKING directory
  # Usage: ...  <name stem> <moses.ini> <testset dir> 
                  #<file suffix> [-t | -n (testset     or normal)]
  FLTINI=$WRKDIR/${CORSTM}.filtered-$TSTSTM/moses.ini
  if [ -f $FLTINI ]; then
    echo "]] Moses already filtered for testset, skipping..."
    echo "]]"
  else
    # prep dir stem name
    prep $TSTDIR $TSTSTM "testset"
    if [ -d $WRKDIR/${CORSTM}.filtered-$TSTSTM ]; then
      echo "]] Removing old filtered folder..."
      rm -r $WRKDIR/${CORSTM}.filtered-$TSTSTM
    fi
    echo "]] FILTERING moses for testset..."
    echo "]]"
    cd $WRKDIR && filter $(dirname $FLTINI) "${TSTSTM}.true.$S" $WRKDIR/mert-work/moses.ini $TSTDIR ||exit 998
  fi

  # !! Differing notions of PIVDIR !!
  # To bleu script, pivdir expects a source file that looks like an outputed translation
  # To this script, pivdir is the dir that needs to be re translated using the
  #   test set that we translate here.
  # This means of the two subsystems in (S -> P -> T), (P->T) must be done first.
  #
  #Usage: translate-for-bleu.sh <ini> <testset dir> <working dir> <stem -true> <s> <t> 
  #           <corpus stem> <pivdir> [-test]
  BLEUNAME="${CORSTM}.bleu-score.$TSTSTM.$S-$T"
  if [ -f $WRKDIR/$BLEUNAME ]; then
    echo "]] BLEU score already exists, skipping..."
    echo "]]"
  else
    echo "]] Translating TESTSET and scoring..."
    echo "]]"
    bleu $FLTINI $TSTDIR $WRKDIR $TSTSTM $S $T $CORSTM "none" "-test"
    echo "]] Bleu score:"
    echo -n "]] "
    cat $WRKDIR/$BLEUNAME
  fi

  if [ $PIVDIR != "none" ]; then
    FLTINI2=$WRKDIR/pivot/${CORSTM}.filtered-$TSTSTM/moses.ini
    if [ -f $FLTINI2 ]; then
      echo "]] Moses already filtered for pivot translation, skipping..."
      echo "]]"
    else
      if [ -d $WRKDIR/pivot ]; then
        echo "]] Removing old pivot filtered folder..."
        rm -r $WRKDIR/pivot
      fi
      echo "]] FILTERING moses for pivot translation..." echo "]]"
      cd $WRKDIR && mkdir pivot && cd pivot || exit 990
      filter $PWD/${CORSTM}.filtered-$TSTSTM "${CORSTM}.${TSTSTM}.translated.$T" $PIVDIR/mert-work/moses.ini $WRKDIR  || exit 999
    fi

    BLEUNAME2="pivot.${CORSTM}.bleu-score.$TSTSTM.$T-en"
    if [ -f $WRKDIR/$BLEUNAME2 ]; then
      echo "]] BLEU score already exists, skipping..."
      echo "]]"
    elif [ $PIVDIR != "none" ]; then
      echo "]] Translating the translation (pivoting)..."
      echo "]]"
      bleu $FLTINI2 $TSTDIR $WRKDIR $TSTSTM $T "en" $CORSTM $WRKDIR "-test"
      echo "]] Original bleu score:"
      echo -n "]] "
      cat $WRKDIR/$BLEUNAME
      echo "]] Pivot bleu score:"
      echo -n "]] "
      cat $WRKDIR/$BLEUNAME2
      echo "]] Baseline bleu score:"
      echo -n "]] "
      BLNAME=$(basename $WRKDIR | head -c 2)-bl.$S-en
      BLBLEUNAME=${BLNAME}.bleu-score.$TSTSTM.$S-en
      cat $WRKDIR/../$BLNAME/$BLBLEUNAME
    fi
  fi

  TD=$(date "+%H:%M %d.%m.%y")

  echo
  echo "==-->> -------------------------- <<--=="
  echo "==-->> NEW SYSTEM SCRIPT COMPLETE <<--=="
  echo "==-->>     $TD    <<--=="
  echo "==-->> -------------------------- <<--=="
  echo
else
  echo "]] $ARG caused chaos! Error!"
  ls $ARG
  exit 2
fi
