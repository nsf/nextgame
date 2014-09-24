#!/bin/bash

mkdir -p builds/clang_debug_asan
pushd builds/clang_debug_asan
cmake ../.. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DNEXTGAME_SANITIZE=address
ninja
popd
ASAN_OPTIONS="detect_leaks=1" ./builds/clang_debug_asan/source/nextgame
