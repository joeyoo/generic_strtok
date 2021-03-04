# Generic strtok_r benchmark (using Google Benchmark)

## Required Installations
- GLIBC
- GCC 4.8
- Clang 3.4
- Visual Studio 14 2015
- Intel 2015 Update 1
- [Google Benchmark](https://github.com/google/benchmark#installation)
- [python scipy library](https://www.scipy.org/scipylib/index.html)

## Required Configuration
1. In ```bench.sh``` and ```compare.sh```, change variables ```$GLIBC``` and ```$GOOGLE_BENCHMARK_PATH``` to the corresponding paths on your machine

## How to run
### To run the full build and compare scripts
Run ``` ./bench.sh ```
### To compare results:
Run ```./compare.sh PATH_TO_RESULT_FILE```, where PATH_TO_RESULT_FILE is something like ```./results/*.json```