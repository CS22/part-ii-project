#!/bin/bash

### Wrapper script for Stanford's Arabic segmenter

if [ $# -lt 2 ] || [ $# -gt 3 ]; then
  echo "$(basename $0) <in> <out> [x gigs mem]"
else

if [ $# -eq 3 ]; then
  RAM=$3
else
  RAM=3
fi

java -mx${RAM}g edu.stanford.nlp.international.arabic.process.ArabicSegmenter \
	-loadClassifier $PJ/libs/sseg/data/arabic-segmenter-atb+bn+arztrain.ser.gz \
	-textFile $1 > $2

fi
