/* Main Benchmark file */

#include "bench-strtok.h"
#include <benchmark/benchmark.h>
#include <fstream>
#include <string.h>

#define OLD_FUNC old_generic_strtok_r
#define NEW_FUNC new_generic_strtok_r

typedef struct __attribute__ ((packed, aligned (MAX_ALIGN)))
{
  char s[BUFSIZE_S];
  char d[BUFSIZE_D];
} stack_buf_t;

static char STATICBUF[STATICBUFSIZE + EPILOGUE_SIZE] alignas (MAX_ALIGN);
static char const *S_PTR = STATICBUF;
static char const *D_PTR = STATICBUF + SIZE_S;

static void
BM_Args (benchmark::internal::Benchmark *b)
{
  for (int size_s = 16; size_s <= SIZE_S; size_s <<= 2)
    for (int size_d = 1; size_d <= SIZE_D && size_d <= size_s; size_d <<= 2)
      b->Args ({ 0, size_s, size_d })
          ->Args ({ 1, size_s, size_d })
          ->Args ({ 2, size_s, size_d })
          ->Args ({ 3, size_s, size_d });
}

#define BM_TEMPLATE(func)                                                     \
  static void BM_##func (benchmark::State &state)                             \
  {                                                                           \
    stack_buf_t buf;                                                          \
    size_t offset = state.range (0), size_s = state.range (1),                \
           size_d = state.range (2), s_f = size_s - 1, d_f = size_d - 1,      \
           cpysize_s = size_s + MAX_ALIGN, cpysize_d = size_d + MAX_ALIGN;    \
    for (auto _ : state)                                                      \
      {                                                                       \
        char *s, *d, *p, *savep;                                              \
        state.PauseTiming ();                                                 \
        s = (char *)memcpy (buf.s, S_PTR, cpysize_s) + offset;                \
        s[s_f] = '\0';                                                        \
        d = (char *)memcpy (buf.d, D_PTR, cpysize_d) + offset;                \
        d[d_f] = '\0';                                                        \
        state.ResumeTiming ();                                                \
        benchmark::DoNotOptimize (p = func (s, d, &savep));                   \
        while (p)                                                             \
          p = func (NULL, d, &savep);                                         \
        benchmark::ClobberMemory ();                                          \
      }                                                                       \
  }                                                                           \
  BENCHMARK (BM_##func)->Apply (BM_Args)

BM_TEMPLATE (OLD_FUNC);
BM_TEMPLATE (NEW_FUNC);

static error_t
read_in_file (std::string path)
{
  std::ifstream is (path);

  if (!is)
    return errno;

  is.rdbuf ()->sgetn (STATICBUF, STATICBUFSIZE);
  is.close ();
  memset (STATICBUF + STATICBUFSIZE, 0, EPILOGUE_SIZE);

  return is.fail () ? errno : 0;
}
#include <iostream>

int
main (int argc, char **argv)
{
  std::string path = PWD;
  path += "/bench-input.txt";

  assert_perror (read_in_file (path));

  ::benchmark::Initialize (&argc, argv);

  assert_perror (::benchmark::ReportUnrecognizedArguments (argc, argv));

  ::benchmark::RunSpecifiedBenchmarks ();
}