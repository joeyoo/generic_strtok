#include "bench-strtok.h"
#include <assert.h>
#include <fstream>
#include <immintrin.h>
#include <iostream>
#include <stdalign.h>

#define FILL_BUF fill_buf_random

// Abbreviation for long, long type name
typedef unsigned long long int ull_t;

// Checks whether V contains a zero-byte
#define HAS0(v)                                                               \
  ~v &(v - UINT64_C (0x0101010101010101)) & UINT64_C (0x8080808080808080)

// Calls _rdrand64_step until an 8-byte sequence w/o zero bytes is generated
#define rnd64(rnd)                                                            \
  do                                                                          \
    _rdrand64_step (&rnd);                                                    \
  while (HAS0 (rnd));

// Char buffer to hold the randomly generated sequence of STATICBUFSIZE bytes
static char BUF[STATICBUFSIZE + EPILOGUE_SIZE] alignas (MAX_ALIGN);

// Fills BUF with random, non-zero bytes using XMM registers + SSE2 instr.
static void
fill_buf_random (void)
{
  size_t i;
  for (i = 0; i < STATICBUFSIZE; i += 16)
    {
      ull_t rnd1, rnd2;
      rnd64 (rnd1);
      rnd64 (rnd2);
      _mm_store_si128 ((__m128i *)(BUF + i), _mm_set_epi64x (rnd1, rnd2));
    }
  memset (BUF + i, 0, 32);
}

static int
write_to_file (std::ofstream os)
{
  if (!os)
    return 1;

  os.rdbuf ()->sputn (BUF, STATICBUFSIZE);
  os.close ();
  return os.fail ();
}

static int
generate_input_file (void)
{
  FILL_BUF ();
  return write_to_file (std::ofstream (INPUT_PATH));
}

int
main (void)
{
  assert_perror (generate_input_file ());
  return 0;
}
