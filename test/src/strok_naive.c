#include "strok.h"

#define uchar unsigned char
/* Getter/Setter macros for accessing/updating table */
#define GET(key) table[(size_t)key]
#define SET(key, val) table[(size_t)key] = val
#define SET4(uptr, val)                                                       \
  SET (uptr[0], 1), SET (uptr[1], 1), SET (uptr[2], 1), SET (uptr[3], 1)

char *
strokr_naive (char *start, const char *delim, char **save_ptr)
{
  uchar table[256], *uptr;
  memset (table, 0, 64);
  memset (table + 64, 0, 64);
  memset (table + 128, 0, 64);
  memset (table + 192, 0, 64);

  uptr = (uchar *)delim; /* unsigned ptr to delim/start */

  if (__glibc_unlikely (uptr[0] == '\0'))
    ;
  else if (uptr[1] == '\0')
    SET (uptr[0], 1);
  else if (uptr[2] == '\0')
    SET (uptr[0], 1), SET (uptr[1], 1);
  else if (uptr[3] == '\0')
    SET (uptr[0], 1), SET (uptr[1], 1), SET (uptr[2], 1);
  else
    {
      SET4 (uptr, 1);
      uptr = PTR_ALIGN_DOWN (uptr, 4) + 4;
      _Bool c0 = uptr[0], c1 = uptr[1], c2 = uptr[2], c3 = uptr[3];

      while (c0 & c1 & c2 & c3)
        {
          SET (uptr[0], 1);
          SET (uptr[1], 1);
          SET (uptr[2], 1);
          SET (uptr[3], 1);

          uptr += 4;

          c0 = uptr[0];
          c1 = uptr[1];
          c2 = uptr[2];
          c3 = uptr[3];
        }

      SET (uptr[0], c0);

      if (c0 & c1)
        {
          SET (uptr[1], 1);
          SET (uptr[2], c2);
        }
    }

  uptr = __glibc_likely (start == NULL) ? (uchar *)*save_ptr : (uchar *)start;

  if (!GET (uptr[0]))
    ;
  else if (!GET (uptr[1]))
    uptr += 1;
  else if (!GET (uptr[2]))
    uptr += 2;
  else if (!GET (uptr[3]))
    uptr += 3;
  else
    {
      int_fast16_t s0, s01, s2, s0123;

      uptr = PTR_ALIGN_DOWN (uptr, 4);
      do
        {
          uptr += 4;

          s0 = GET (uptr[0]);
          s01 = s0 & GET (uptr[1]);
          s2 = GET (uptr[2]);
          s0123 = s01 & s2 & GET (uptr[3]);
        }
      while (s0123);

      uptr += !s01 ? s0 : s2 + 2;
    }

  if (__glibc_unlikely (*uptr == '\0'))
    return *save_ptr = (char *)uptr, NULL;

  start = (char *)uptr;

  /* table['\0'] must be set for '\0' to continue being a breaking condition */
  SET ('\0', 1);

  if (GET (uptr[0]))
    ;
  else if (GET (uptr[1]))
    uptr += 1;
  else if (GET (uptr[2]))
    uptr += 2;
  else if (GET (uptr[3]))
    uptr += 3;
  else
    {
      int_fast16_t s0, s01, s2, s0123;

      uptr = PTR_ALIGN_DOWN (uptr, 4);
      do
        {
          uptr += 4;

          s0 = GET (uptr[0]);
          s01 = s0 | GET (uptr[1]);
          s2 = GET (uptr[2]);
          s0123 = s01 | s2 | GET (uptr[3]);
        }
      while (!s0123);

      if (s01)
        uptr += 1 - s0;
      else
        uptr += 3 - s2;
    }

  {
    char *end = (char *)uptr;

    if (__glibc_likely (*end != '\0'))
      *end++ = 0;

    *save_ptr = end;

    return start;
  }
}

char *
strok_naive (char *s, const char *d)
{
  static char *p;

  return strokr_naive (s, d, &p);
}
