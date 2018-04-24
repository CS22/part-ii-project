#!/bin/bash

## Script for translating n best

set -e

echo "-=_nbest_script_=-"

usage() {
  echo "Usage: `basename $0` <S-P ini> <P-T ini> <translation name> <n> <source file> <ref file>"
}

ARGC=6

if [ $# -ne $ARGC ]; then
  usage
  exit 1
fi

SPINI=$1
PTINI=$2
TNAME=$3
    N=$4
SOURCE=$5
REF=$6

PIVLS="temp.${N}.list"
echo "$TNAME"

if [ "$(ls temp.* 2> /dev/null)" ]; then
  rm temp.*
fi

echo
echo "Translating source..."
$MOSES -f $SPINI -n-best-list $PIVLS $N distinct < $SOURCE 2>"${PIVLS}.out" >/dev/null
echo "Source translated"

I=0; LN=0; ACTLN=0
while : ; do

  if [ $LN -eq $ACTLN ]; then
    PLINE=$LINE
    if ! read LINE; then
      break;
    fi
    ACTLN=$(echo $LINE |awk '{ print $1 }') # actual line number
    LINE=$(getSens <<< "$LINE")
  fi

  if [ $I -eq $N ]; then I=0; LN=$(($LN+1)); fi

  if [ $LN -eq $ACTLN ]; then
#    echo "File: temp.$I; Line: $(echo $LINE |head -c 12)"
    echo $LINE >> temp.$I
  else
#    echo "File: temp.$I; Line: $(echo $PLINE |head -c 12)"
    echo $PLINE >> temp.$I
  fi

  I=$((I+1))

done < $PIVLS

echo "Translating the $N..."

for I in $(seq 0 $(( $N-1 )) ); do
  echo -n -e "$I\t"
  $MOSES --threads 8 -f $PTINI < temp.$I > n${I}.$TNAME 2>n${I}.${TNAME}.out
done
echo; echo

for I in $(seq 0 $(( $N-1 )) ); do
  echo -e -n "$I\t"
  bleu-score.sh $REF n${I}.$TNAME | tee ${TNAME}.${I}.bleu
done
echo; echo
