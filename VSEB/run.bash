#!/bin/bash
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        bazel run --action_env CC=/usr/bin/clang-12 //:Edison --cxxopt='-std=c++20'
elif [[ "$OSTYPE" == "darwin"* ]]; then
        bazel run //:Edison --cxxopt='-std=c++20' --cpu=darwin_arm64
else
	echo "UNKNOWN OS... EXITING..."
fi

#Final Program Termination Guard
echo -e "SYSTEM EXITED WITH (TO THIS BASH PROGRAM) UNKNOWN ERROR... RUN.BASH SHOULD NOT HAVE GOTTEN HERE. CHECK SCREEN FOR OUTPUT ERROR" >> Emergency_Buy_Log.txt 
