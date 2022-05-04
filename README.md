# CTPEG
A compiletime parser generator based on parsing expression grammars

## Dependencies

Run this command to fetch dependencies
```sh
git clone https://github.com/TartanLlama/expected.git deps
```

## Build

To build the example run

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Testing

To build tests run

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DINCLUDE_TESTS=Yes
cmake --build build
```

## Testing with spdlog

In order to test how the addition of the library affects compilation time, follow the following steps
```sh
https://github.com/gabime/spdlog && cd spdlog && git checkout v1.10.0

# ccache --clear # Only necessary if you use ccache
cmake -B build -DCMAKE_BUILD_TYPE=Release
time cmake --build build 
# record the time taken to build without ctpeg

git reset --hard && git clean -d -f -f -x
git apply ../testing/spdlog_test.patch
# ccache --clear
cmake -B build -DCMAKE_BUILD_TYPE=Release
time cmake --build build
# record the time taken to build with ctpeg
```

If unit tests compile, they have succeeded

## Usage

Add `ctpeg.hpp` to your project and add `expected/include` to the include paths.
