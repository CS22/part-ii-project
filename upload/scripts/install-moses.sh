#!/bin/bash

# Installs Moses and all dependencies 
# should have $PJ etc already defined

set -e

echo "[:}}} INSTALLING DEPENDENCIES {{{:]"
sudo apt-get install build-essential git-core pkg-config automake libtool wget zlib1g-dev python-dev libbz2-dev g++ subversion liblzma-dev python-dev graphviz imagemagick make cmake libgoogle-perftools-dev libboost-all-dev
cd $PJ
echo "[:} CLONING MOSES {:]"
if [ -d mosesdecoder ]; then
  echo "[:} mosesdecoder already cloned {:]"
else 
  git clone https://github.com/moses-smt/mosesdecoder.git
fi

cd mosesdecoder
echo "[:} INSTALLING ADDITIONAL DEPENDENCIES {:]"
make -f contrib/Makefiles/install-dependencies.gmake
echo "[:} COMPILING MOSES {:]"
./compile.sh -j12

echo "[:} TESTING MOSES: {:]"
wget http://www.statmt.org/moses/download/sample-models.tgz
tar xzf sample-models.tgz
cd sample-models
cd $MO/sample-models
echo "[:} RUNNING TRANSLATION {:]"
$MO/bin/moses -f phrase-model/moses.ini < phrase-model/in > out

echo "[[[:} FINISHED MOSES INSTALL {:]]]"
install-mgiza.sh
echo "[:::} INSTALL COMPLETED {:::]"
