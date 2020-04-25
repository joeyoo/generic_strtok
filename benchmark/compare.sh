#!/bin/sh

src/benchmark/tools/compare.py filters $1 BM_OLD_FUNC BM_NEW_FUNC | sed -r "s/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g" | python avg_perf.py