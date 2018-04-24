#!/bin/bash

## Utility script for generating 55,000 sentence baseline direct translation
## systems

for lang in ar es ru zh; do
  STEM=p0-bl.$lang-en
  DIR=$PJ/corpus/unpc/$STEM
  if ! [ -d $DIR ]; then mkdir $DIR; fi
  cat $PJ/corpus/unpc/bl.ctrl | sed "s/LANG/$lang/g" > $DIR/$STEM.ctrl
  ln -f $PJ/corpus/unpc/55k/unpc-55k.true.$lang $DIR/$STEM.true.$lang
  ln -f $PJ/corpus/unpc/55k/unpc-55k.true.en $DIR/$STEM.true.en
  $PJ/scripts/new-system.sh $DIR/$STEM.ctrl
done
