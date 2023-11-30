#!/bin/bash

if [ -d ./Release ] ; then
  rm -r ./Release
fi

mkdir ./Release
cp ErrorMsg.txt ./Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc ..
cd ..

if [ -d ./Debug ] ; then
  rm -r ./Debug
fi

mkdir ./Debug
cp ErrorMsg.txt ./Debug
cd Debug
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc .. 
cd ..

