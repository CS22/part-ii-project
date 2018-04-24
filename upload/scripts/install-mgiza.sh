#!/bin/bash

# SCRIPT TO INSTALL MULTI THREADED GIZA

set -e

echo ">>o> MGIZA INSTALLATION SCRIPT <<o<"
cd $PJ/libs
if [ -d mgiza ]; then
  echo ">>o> mgiza already cloned <<o<"
else
  git clone https://github.com/moses-smt/mgiza.git
fi

export BOOST_ROOT=/usr/include
export BOOST_LIBRARYDIR=$BOOST_ROOT/boost

cd mgiza/mgizapp
echo ">>o> Making... <o<<"
cmake .
make
make install

echo ">>o> Moving scripts and bins to moses tool dir <<o<"
if [ -d $MO/tools ]; then
  echo ">>o> Moses tool dir already exists, continuing <<o<"
else
  mkdir $MO/tools
fi

cp inst/scripts/* inst/bin/* $MO/tools

echo ">>o> MGIZA INSTALLATION COMPLETE <<o<"
