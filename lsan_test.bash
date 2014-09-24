#!/bin/bash

mkdir -p builds/clang_debug_asan
pushd builds/clang_debug_asan
cmake ../.. -GNinja -DCMAKE_BUILD_TYPE=Debug -DNEXTGAME_SANITIZE=address
ninja
ASAN_OPTIONS="detect_leaks=1" ninja test
popd
