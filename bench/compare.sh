#!/bin/sh
# Replace with path of where you installed google benchmark
GOOGLE_BENCHMARK_PATH=./benchmark;

${GOOGLE_BENCHMARK_PATH}/tools/compare.py filters $1 BM_OLD_FUNC BM_NEW_FUNC | sed -r "s/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g" | python ./tools/avg_perf.py