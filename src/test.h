#ifndef _BENCH_H_
#define _GNU_SOURCE
#define _BENCH_H_
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#include "strok.h"
#include "print.h"

#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <tgmath.h>
#include <time.h>
#include <unistd.h>


int should_print, should_count, total_tests, num_correct, num_incorrect;

#define RESUME_COUNTING should_count = 1;
#define STOP_COUNTING should_count = 0;

#define RESUME_PRINTING should_print = 1;
#define STOP_PRINTING should_print = 0;

void time_test (void (*) (), int, const char *);
int strcmp_test (const char *s1, const char *s2);

#define ITERATIONS 1000000
#define WARMUP_ITER 10000

static void
assert_equal_str (char *expected, char *actual, const char *message)
{
  if (!should_count)
    return;

  ++total_tests;

  if (expected == NULL || actual == NULL)
    {
      if (expected == NULL && actual == NULL)
        {
          ++num_correct;
        }
      else
        {
          ++num_incorrect;
          if (should_print)
            // printf ("[FAILED Test %d: %s]: Expected %s, actual %s\n",
            //         total_tests, message, expected,
            //         actual == NULL ? "" : actual);
             prints_hex (expected, actual);
        }
      return;
    }

  if (!strcmp (expected, actual))
    {
      ++num_correct;
    }
  else
    {
      ++num_incorrect;
      if (should_print)
        prints_hex (expected, actual);
        // printf ("[FAILED Test %d: %s]: Expected %s, actual %s\n", total_tests,
        //         message, expected, actual);
    }
}

#define WARMUP(f)                                                             \
  for (int i = 0; i < WARMUP_ITER; ++i)                                       \
    f ();

#define HAS0(v) ~v &(v - 0x0101010101010101UL) & 0x8080808080808080UL
#define RND64(rnd)                                                            \
  do                                                                          \
    _rdrand64_step (&rnd);                                                    \
  while (HAS0 (rnd))
#endif