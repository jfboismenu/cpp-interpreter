// Copyright (c) 2023 Jean-François Boismenu

mkdir -p build
clang luno-cpp/*.cpp -o build/luno -std=c++17 -I . -lstdc++
build/luno $@
