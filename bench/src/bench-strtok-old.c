#define _GNU_SOURCE
#include "bench-strtok.h"
#include <string.h>

#define WITH_INLINE

#ifdef WITH_INLINE
#define INLINE __always_inline
#else
#define INLINE __attribute_noinline_
#endif

/*
 * Things to note:
 *     1. We want to specify "__attribute_noinline__" because the compiler will
 *        likely inline and further optimize the calls to generic_str(c)spn,
 *        which isn't indicative of what happens in a default build of glibc
 *        (-O2/s -g)
 *     2. Let's say we compiled glibc with LTO (Link-Time Optimization).
 *        Although the redundant allocation for the 256-byte table is optimized
 *        away, all the logic that deals with initializing, getting, and
 *        setting; remains.
 */

INLINE size_t generic_strspn (const char *str, const char *accept);
INLINE size_t generic_strcspn (const char *str, const char *reject);

/* Return the length of the maximum initial segment
   of S which contains only characters in ACCEPT.  */
size_t
generic_strspn (const char *str, const char *accept)
{
  if (accept[0] == '\0')
    return 0;
  if (__glibc_unlikely (accept[1] == '\0'))
    {
      const char *a = str;
      for (; *str == *accept; str++)
        ;
      return str - a;
    }

  /* Use multiple small memsets to enable inlining on most targets.  */
  unsigned char table[256];
  unsigned char *p = (unsigned char *)memset (table, 0, 64);
  memset (p + 64, 0, 64);
  memset (p + 128, 0, 64);
  memset (p + 192, 0, 64);

  unsigned char *s = (unsigned char *)accept;
  /* Different from strcspn it does not add the NULL on the table
     so can avoid check if str[i] is NULL, since table['\0'] will
     be 0 and thus stopping the loop check.  */
  do
    p[*s++] = 1;
  while (*s);

  s = (unsigned char *)str;
  if (!p[s[0]])
    return 0;
  if (!p[s[1]])
    return 1;
  if (!p[s[2]])
    return 2;
  if (!p[s[3]])
    return 3;

  s = PTR_ALIGN_DOWN (s, 4);

  unsigned int c0, c1, c2, c3;
  do
    {
      s += 4;
      c0 = p[s[0]];
      c1 = p[s[1]];
      c2 = p[s[2]];
      c3 = p[s[3]];
    }
  while ((c0 & c1 & c2 & c3) != 0);

  size_t count = s - (unsigned char *)str;
  return (c0 & c1) == 0 ? count + c0 : count + c2 + 2;
}

/* Return the length of the maximum initial segment of S
   which contains no characters from REJECT.  */
size_t
generic_strcspn (const char *str, const char *reject)
{
  if (__glibc_unlikely (reject[0] == '\0')
      || __glibc_unlikely (reject[1] == '\0'))
    return (size_t)__strchrnul (str, reject[0]) - (size_t)str;

  /* Use multiple small memsets to enable inlining on most targets.  */
  unsigned char table[256];
  unsigned char *p = (unsigned char *)memset (table, 0, 64);
  memset (p + 64, 0, 64);
  memset (p + 128, 0, 64);
  memset (p + 192, 0, 64);

  unsigned char *s = (unsigned char *)reject;
  unsigned char tmp;
  do
    p[tmp = *s++] = 1;
  while (tmp);

  s = (unsigned char *)str;
  if (p[s[0]])
    return 0;
  if (p[s[1]])
    return 1;
  if (p[s[2]])
    return 2;
  if (p[s[3]])
    return 3;

  s = PTR_ALIGN_DOWN (s, 4);

  unsigned int c0, c1, c2, c3;
  do
    {
      s += 4;
      c0 = p[s[0]];
      c1 = p[s[1]];
      c2 = p[s[2]];
      c3 = p[s[3]];
    }
  while ((c0 | c1 | c2 | c3) == 0);

  size_t count = s - (unsigned char *)str;
  return (c0 | c1) != 0 ? count - c0 + 1 : count - c2 + 3;
}

char *
old_generic_strtok_r (char *s, const char *delim, char **save_ptr)
{
  char *end;

  if (s == NULL)
    s = *save_ptr;

  if (*s == '\0')
    {
      *save_ptr = s;
      return NULL;
    }

  /* Scan leading delimiters.  */
  // s += CALLSPN (_generic_strspn, s, delim);
  s += generic_strspn (s, delim);
  if (*s == '\0')
    {
      *save_ptr = s;
      return NULL;
    }

  /* Find the end of the token.  */
  // end = s + CALLSPN (_generic_strcspn, s, delim);
  end = s + generic_strcspn (s, delim);
  if (*end == '\0')
    {
      *save_ptr = end;
      return s;
    }

  /* Terminate the token and make *SAVE_PTR point past it.  */
  *end = '\0';
  *save_ptr = end + 1;
  return s;
}