#!/bin/bash

## Script to batch call METEOR script. Runs in parallel. MEMTOTAL,
## the total system memory, and PARAC, the number of threads to run
## in parallel, should be adjusted for machine and filesizes.

set -e

echo "Batch run of METEOR script"
echo "Main server version"

usage() {
  echo "Usage: `basename $0` <basedir> <min num> <max num> <suffix> <prefix (eg p0)>"
}

ARGC=5

if [ $# -ne $ARGC ]; then
  usage
  exit 1
fi

MEMTOTAL=$((1024 * 3))

DIR=$1
MIN=$2
MAX=$3
SUF=$4
PRF=$5
METEOR="/home/cos/libs/meteor-1.5/meteor-1.5.jar"

if [ $SUF = "-" ]; then
  SUF="testset.pivot.translated.en";
fi

if [ $PRF = "-" ]; then
  PRF="p0";
fi

if ! [ -d $DIR ]; then
  echo "Not a dir!"
  usage
  exit 2
fi

if ! [ $MAX -gt 0 ]; then
  usage "Max must be positive!"
  exit 3
fi

PRIMDIR=$DIR/prim
SCNDDIR=$DIR/scnd
ALNSDIR=$DIR/alns

L=$(basename $DIR)

PARAC=3

#MEMMAX=$(( $MEMTOTAL / $(( 4*($MAX+1 -$MIN) )) ))
MEMMAX=$(( $MEMTOTAL / $PARAC ))
TOTAL=$(( 4*( $MAX+1 - $MIN ) )) # 4 languages worth of total
P=0; IP=0;

if [ $MEMMAX -eq 0 ]; then
  echo "Not enough memory to do in parallel..."
  echo -e "This might take a while...\n"
else
  echo "Memory max (per instance): $MEMMAX"
fi

progressbar() {

        echo; printf "%.3f" $P; echo -n "% ["
        for perc in $( seq 0 2 100); do
          if [ $perc -lt $IP ]; then echo -n "="
          else echo -n " "; fi
        done
        echo -n "] ($NOW) "; date "+%H:%M"; echo
}

COUNT=0;
NOW=$(date "+%H:%M")
for LNG in ar es fr ru zh; do
  if [ $LNG != $L ]; then
    for I in $( seq $MIN $PARAC $MAX ); do
      if [ $MEMMAX -gt 0 ]; then 
        for J in $( seq $I $(( $I + $PARAC-1 )) ); do
          if [ $J -gt $MAX ]; then break; fi
          echo -e -n "$J $LNG\t"
          java -Xmx${MEMMAX}M -cp $METEOR Matcher \
            $SCNDDIR/scnd.${J}.${PRF}.${LNG}-${L}.${SUF} \
            $PRIMDIR/prim.${J}.${PRF}.${LNG}-${L}.${SUF} \
          > $ALNSDIR/alns.${J}.${PRF}.${LNG}-${L}.${SUF} &
          PIDS[$J]=$!
          COUNT=$(( $COUNT + 1 ))
        done; echo; 

        if [ $(printf "%.0f" $P) -gt $IP ]; then
          IP=$(printf "%.0f" $P)
        fi

        progressbar

        for K in $( seq $I $(( $I + $PARAC-1 )) ); do
          wait ${PIDS[$K]}
        done

        P=$( bc -l <<< "100*$COUNT/$TOTAL")

      else
        echo "Something has gone seriously wrong! MEMMAX: $MEMMAX"
        exit 4
      fi
      OLDI=$I
    done
  fi
done


progressbar
echo "Done!"
