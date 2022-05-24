#!/bin/bash
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        bazel build --action_env CC=/usr/bin/clang-12 //:Emergency_Stop --cxxopt='-std=c++20'
        chmod a+x /home/aidanjalili03/Edison-Live/Edison/Emergency_Stop/bazel-bin/Emergency_Stop
elif [[ "$OSTYPE" == "darwin"* ]]; then
        bazel build //:Emergency_Stop --cxxopt='-std=c++20' --cpu=darwin_arm64
        chmod a+x /Users/aidanjalili03/Desktop/Edison/Emergency_Stop/bazel-bin/Emergency_Stop
else
	echo "UNKNOWN OS... EXITING..."
fi