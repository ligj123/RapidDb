#!/bin/bash

if [ -d ./Release ] ; then 
  rm -r ./Release
fi

mkdir ./Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ..
cd ..

if [ -d ./Debug ] ; then 
  rm -r ./Debug
fi

mkdir ./Debug
cd Debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ .. 
cd ..

