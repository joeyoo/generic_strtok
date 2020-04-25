#!/bin/sh
GLIBC=/home/user/glibc/build;
BASELINE=bench-strtok-new;
CONTENDER=bench-strtok-old;

BENCHMARK_REPETITIONS=12;
BENCHMARK_OUT_FORMAT=json;
BENCHMARK_OUT="results/$(date '+%y%m%d_%H%M%S').${BENCHMARK_OUT_FORMAT}";
BENCHMARK_REPORT_AGGREGATES_ONLY=true;
BENCHMARK_DISPLAY_AGGREGATES_ONLY=true;
BENCHMARK_FORMAT=console;

g++ \
src/generate-input.cc \
-std=gnu++11 -mrdrnd -o generate-input \
&& ./generate-input \
&& \

gcc \
-Wl,-rpath=${GLIBC}:${GLIBC}/math:${GLIBC}/elf:${GLIBC}/dlfcn:${GLIBC}/nss:${GLIBC}/nis:${GLIBC}/rt:${GLIBC}/resolv:${GLIBC}/crypt:${GLIBC}/nptl:${GLIBC}/dfp -Wl,--dynamic-linker=${GLIBC}/elf/ld.so \
-O2 -c -shared -fPIC -Wall \
"src/${BASELINE}.c" \
-o "build/${BASELINE}.so" \
&& \
gcc \
-Wl,-rpath=${GLIBC}:${GLIBC}/math:${GLIBC}/elf:${GLIBC}/dlfcn:${GLIBC}/nss:${GLIBC}/nis:${GLIBC}/rt:${GLIBC}/resolv:${GLIBC}/crypt:${GLIBC}/nptl:${GLIBC}/dfp -Wl,--dynamic-linker=${GLIBC}/elf/ld.so \
-O2 -c -shared -fPIC -Wall \
"src/${CONTENDER}.c" \
-o "build/${CONTENDER}.so" \
&& \

g++ \
src/bench-strtok.cc \
-std=gnu++11 -lbenchmark -lpthread \
-o bench \
"build/${BASELINE}.so" \
"build/${CONTENDER}.so" \
&& \

./bench \
--benchmark_out_format=${BENCHMARK_OUT_FORMAT} \
--benchmark_format=${BENCHMARK_FORMAT} \
--benchmark_out=${BENCHMARK_OUT} \
--benchmark_repetitions=${BENCHMARK_REPETITIONS} \
--benchmark_display_aggregates_only=${BENCHMARK_DISPLAY_AGGREGATES_ONLY} \
--benchmark_report_aggregates_only=${BENCHMARK_REPORT_AGGREGATES_ONLY} \
&& src/benchmark/tools/compare.py filters ${BENCHMARK_OUT} BM_OLD_FUNC BM_NEW_FUNC && ./compare.sh ${BENCHMARK_OUT}