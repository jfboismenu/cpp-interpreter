# Copyright (c) 2023 Jean-François Boismenu

set -e

mkdir -p build
clang luno-cpp/*.cpp -o build/luno -std=c++2a -I . -lstdc++
build/luno $@
