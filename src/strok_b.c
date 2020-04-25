#include "strok.h"

#define NUL ((char)0)
#ifndef __u_char_defined
typedef unsigned char u_char;
typedef unsigned long int u_long;
#endif

#define EXPAND_HEX(type, mask)                                                \
  ((sizeof (type) > 4) ? 0x##mask##mask : 0x##mask)
#define ZMASK(type, word)                                                     \
  ~word &(word - EXPAND_HEX (type, 01010101)) & EXPAND_HEX (type, 80808080)
#define handle_zmask(table, shft0, fst2_nonzero, shft2)                       \
  {                                                                           \
    table[d[0]] = (u_char) (zmask >> shft0) ^ 0x01;                           \
    if (fst2_nonzero)                                                         \
      table[d[1]] = 1, table[d[2]] = (u_char) (zmask >> shft2) ^ 0x01;        \
  }

char *
strokr_b (char *start, const char *delim, char **save_ptr)
{
  char *end;
  u_char *d, *s;

  d = (u_char *)delim;
  s = __glibc_likely (start == NULL) ? (u_char *)*save_ptr : (u_char *)start;

  if ((d[0] == NUL) | (d[1] == NUL))
    {
      const int dc = (char)d[0];

      if (__glibc_unlikely (*s == NUL))
        return *save_ptr = (char *)s, NULL;

      while (*s == dc)
        ++s;

      if (__glibc_unlikely (*s == NUL))
        return *save_ptr = (char *)s, NULL;

      start = (char *)s;

      s = (u_char *)strchrnul ((char *)s, dc);
    }
  else
    {
      u_char table[256];
      memset (table, 0, 64);
      memset (table + 64, 0, 64);
      memset (table + 128, 0, 64);
      memset (table + 192, 0, 64);

      table[d[0]] = table[d[1]] = 1;

      if ((d[2] == NUL) | (d[3] == NUL))
        table[d[2]] = d[2] != NUL;
      else
        {
          uint_fast32_t word, zmask;

          table[d[2]] = table[d[3]] = 1;

          d = PTR_ALIGN_DOWN (d, 4) + 4;

          word = *(uint32_t *)d;
          zmask = ZMASK (uint32_t, word);

          if (sizeof word > 4)
            if (zmask == 0)
              {
                table[d[0]] = table[d[1]] = table[d[2]] = table[d[3]] = 1;
                word = *(uint_fast32_t *)(d = PTR_ALIGN_UP (d, 8));

                zmask = ZMASK (uint_fast32_t, word);
              }

          while (zmask == 0)
            {
              table[d[0]] = table[d[1]] = table[d[2]] = table[d[3]] = 1;
              d += 4;

              if (sizeof word > 4)
                {
                  table[d[0]] = table[d[1]] = table[d[2]] = table[d[3]] = 1;
                  d += 4;
                }

              word = *(uint_fast32_t *)d;
              zmask = ZMASK (uint_fast32_t, word);
            }

#if UINT_FAST32_WIDTH > 32
          if ((uint32_t)zmask == 0) // nonzero first 4
            {
              table[d[0]] = table[d[1]] = table[d[2]] = table[d[3]] = 1;
              d += 4;
              zmask >>= 32;
            }
#endif
          handle_zmask (table, 7, (uint16_t)zmask == 0x0000, 23);
        }

      /* Basically strspn */
      if (!table[s[0]])
        ;
      else if (!table[s[1]])
        s += 1;
      else if (!table[s[2]])
        s += 2;
      else if (!table[s[3]])
        s += 3;
      else
        {
          u_long s0, s0_s1, s2;

          s = PTR_ALIGN_DOWN (s, 4);
          do
            {
              s += 4;

              s0 = table[s[0]];
              s0_s1 = s0 & table[s[1]];
              s2 = table[s[2]];
            }
          while (s0_s1 & s2 & table[s[3]]);

          s += !s0_s1 ? s0 : s2 + 2;
        }

      if (__glibc_unlikely (*s == NUL))
        return *save_ptr = (char *)s, NULL;

      start = (char *)s;

      /* table[NUL] must be set for NUL to continue being a breaking
       * condition */
      table[NUL] = 1;

      if (table[s[0]])
        ;
      else if (table[s[1]])
        s += 1;
      else if (table[s[2]])
        s += 2;
      else if (table[s[3]])
        s += 3;
      else
        {
          u_long s0, s0_s1, s2;

          s = PTR_ALIGN_DOWN (s, 4);
          do
            {
              s += 4;

              s0 = table[s[0]];
              s0_s1 = s0 | table[s[1]];
              s2 = table[s[2]];
            }
          while (!(s0_s1 | s2 | table[s[3]]));

          if (s0_s1)
            s += 1 - s0;
          else
            s += 3 - s2;
        }
    }

  end = (char *)s;

  if (__glibc_likely (*end != NUL))
    *end++ = NUL;

  *save_ptr = end;

  return start;
}

char *
strok_b (char *str, const char *d)
{
  static char *p;

  return strokr_b (str, d, &p);
}
