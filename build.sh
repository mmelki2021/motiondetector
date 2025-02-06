#!/bin/sh
rm -rf ./build_test && mkdir ./build_test && cd ./build_test && cmake .. && make all
