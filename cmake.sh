#!/bin/bash

if [ -d ./release ] ; then 
  rm -r ./release
fi

mkdir ./release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ..
cd ..

if [ -d ./debug ] ; then 
  rm -r ./debug
fi

mkdir ./debug
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ .. 
cd ..

