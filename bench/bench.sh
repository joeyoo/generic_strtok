#!/bin/sh

# Change GLIBC and GOOGLE_BENCHMARK_PATH to where their corresponding paths on your machine
GLIBC=/home/user/glibc/build;
GOOGLE_BENCHMARK_PATH=./benchmark;

# Configure number of repetitions
BENCHMARK_REPETITIONS=10;

BASELINE=bench-strtok-new;
CONTENDER=bench-strtok-old;
BENCHMARK_OUT_FORMAT=json;
BENCHMARK_OUT="results/$(date '+%y%m%d_%H%M%S').${BENCHMARK_OUT_FORMAT}";
BENCHMARK_REPORT_AGGREGATES_ONLY=true;
BENCHMARK_DISPLAY_AGGREGATES_ONLY=true;
BENCHMARK_FORMAT=console;

# build and run generate-input
g++ \
src/generate-input.cc \
-std=gnu++11 -mrdrnd -o ./build/generate-input \
&& ./build/generate-input \
&& \

# build and run both strtok functions
gcc \
-Wl,-rpath=${GLIBC}:${GLIBC}/math:${GLIBC}/elf:${GLIBC}/dlfcn:${GLIBC}/nss:${GLIBC}/nis:${GLIBC}/rt:${GLIBC}/resolv:${GLIBC}/crypt:${GLIBC}/nptl:${GLIBC}/dfp -Wl,--dynamic-linker=${GLIBC}/elf/ld.so \
-O2 -c -shared -fPIC -Wall \
"./src/${BASELINE}.c" \
-o "./build/${BASELINE}.so" \
&& \
gcc \
-Wl,-rpath=${GLIBC}:${GLIBC}/math:${GLIBC}/elf:${GLIBC}/dlfcn:${GLIBC}/nss:${GLIBC}/nis:${GLIBC}/rt:${GLIBC}/resolv:${GLIBC}/crypt:${GLIBC}/nptl:${GLIBC}/dfp -Wl,--dynamic-linker=${GLIBC}/elf/ld.so \
-O2 -c -shared -fPIC -Wall \
"./src/${CONTENDER}.c" \
-o "./build/${CONTENDER}.so" \
&& \

# build bench executable
g++ \
src/bench-strtok.cc \
-std=gnu++11 -lbenchmark -lpthread \
-o ./build/bench \
"./build/${BASELINE}.so" \
"./build/${CONTENDER}.so" \
&& \

# run bench 
./build/bench \
--benchmark_out_format=${BENCHMARK_OUT_FORMAT} \
--benchmark_format=${BENCHMARK_FORMAT} \
--benchmark_out=${BENCHMARK_OUT} \
--benchmark_repetitions=${BENCHMARK_REPETITIONS} \
--benchmark_display_aggregates_only=${BENCHMARK_DISPLAY_AGGREGATES_ONLY} \
--benchmark_report_aggregates_only=${BENCHMARK_REPORT_AGGREGATES_ONLY} \
&& ${GOOGLE_BENCHMARK_PATH}/tools/compare.py filters ${BENCHMARK_OUT} BM_OLD_FUNC BM_NEW_FUNC && ./compare.sh ${BENCHMARK_OUT}