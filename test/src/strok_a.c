#include "strok.h"

#define NUL ((char)0)
typedef unsigned char u_char;
typedef unsigned long int u_long;

char *
strokr_a (char *start, const char *delim, char **save_ptr)
{
  char *end;
  u_char *d = (u_char *)delim, *s = __glibc_likely (start == NULL)
                                        ? (u_char *)*save_ptr
                                        : (u_char *)start;

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
        table[d[2]] = (d[2] != NUL);
      else
        {
#if defined INT16_MAX && defined INT32_MAX
          uint_fast32_t word, zmask;

          table[d[2]] = table[d[3]] = 1;

          word = *(uint32_t *)(d = PTR_ALIGN_DOWN (d, 4) + 4);

          while ((zmask = ~word & (word - 0x01010101) & 0x80808080)
                 == 0x00000000)
            {
              table[d[0]] = table[d[1]] = table[d[2]] = table[d[3]] = 1;
              word = *(uint32_t *)(d += 4);
            }

/* Move MSB to LSB and XOR to get (in bits):
      0..1 from 0..0
      0..0 from 1..0,
    effectively doing (!!) w/o flag dependency.
    fst2_nonzero is true if the first 2 bytes of s are nonzero */
#define handle_zmask(shft0, fst2_nonzero, shft2)                              \
  {                                                                           \
    table[d[0]] = (u_char) (zmask >> shft0) ^ 0x01;                           \
    if (fst2_nonzero)                                                         \
      table[d[1]] = 1, table[d[2]] = (u_char) (zmask >> shft2) ^ 0x01;        \
  }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
          handle_zmask (7, (uint16_t)zmask == 0x0000, 23);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
          handle_zmask (31, zmask <= 0x8080, 15);
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
          handle_zmask (23, zmask <= 0x8080, 7);
#endif /* END __BYTE_ORDER__ == __ORDER_X_ENDIAN */
#else  /* Portably do it in bytes */
          u_long c0 = d[0] != NUL, c1 = d[1] != NUL, c2 = d[2] != NUL,
                 c3 = d[3] != NUL;

          while (c0 & c1 & c2 & c3)
            {
              table[d[0]] = table[d[1]] = table[d[2]] = table[d[3]] = 1;

              d += 4;

              c0 = d[0] != NUL;
              c1 = d[1] != NUL;
              c2 = d[2] != NUL;
              c3 = d[3] != NUL;
            }

          table[d[0]] = c0;

          if (!(c0 & c1))
            {
              table[d[1]] = 1;
              table[d[2]] = c2;
            }
#endif /* END __INT_LEAST32_WIDTH__ == 32 && __CHAR_BIT__ == 8 */
        }

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
strok_a (char *s, const char *d)
{
  static char *p;

  return strokr_a (s, d, &p);
}
