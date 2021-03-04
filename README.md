# Generic Strtok

```strtok``` is an infamously standard C-library function that tokenizes a string based on a given set (that's not really a set, but a string) of delimiting characters. You can read more about it [here](https://pubs.opengroup.org/onlinepubs/007904975/functions/strtok.html). In most implementations (including GLIBC), it simply wraps a call to the thread-safe version called ```strtok_r``` which is the ```_r```eentrant version of ```strtok```. Therefore, you'll see that my optimization is for ```strtok_r```.

## Let's start with GLIBC's current implementation:
### How it works
To find the beginning of the token, ```strtok_r``` first skips over the leading delimiters by calling ```strspn```, which is another string function that counts the number of consecutive characters that consist entirely of characters in the delimiter string. Having found the beginning of the token, we then find the end of the token by calling ```strcspn```, which returns the length of consecutive characters that DON'T consist of any characters in the delimiter string. Along with some checks for the NUL character (which indicates the end of a string), that's pretty much it for finding the token. Continue below to see how strspn/strcspn are implemented.
```c
/* Parse S into tokens separated by characters in DELIM.
   If S is NULL, the saved pointer in SAVE_PTR is used as
   the next starting point.  For example:
	char s[] = "-abc-=-def";
	char *sp;
	x = strtok_r(s, "-", &sp);	// x = "abc", sp = "=-def"
	x = strtok_r(NULL, "-=", &sp);	// x = "def", sp = NULL
	x = strtok_r(NULL, "=", &sp);	// x = NULL
		// s = "abc\0-def\0"
*/
char *
__strtok_r (char *s, const char *delim, char **save_ptr)
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
  s += strspn (s, delim);
  if (*s == '\0')
    {
      *save_ptr = s;
      return NULL;
    }

  /* Find the end of the token.  */
  end = s + strcspn (s, delim);
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
```

### strspn
Among the other clever techniques to optimize this function, you'll notice the 256-byte ```table``` used to act as a set of the delimiting characters. As you'll see below, ```strcspn``` uses this same technique.
```c
/* Return the length of the maximum initial segment
   of S which contains only characters in ACCEPT.  */
size_t
STRSPN (const char *str, const char *accept)
{
  if (accept[0] == '\0')
    return 0;
  if (__glibc_unlikely (accept[1] == '\0'))
    {
      const char *a = str;
      for (; *str == *accept; str++);
      return str - a;
    }

  /* Use multiple small memsets to enable inlining on most targets.  */
  unsigned char table[256];
  unsigned char *p = memset (table, 0, 64);
  memset (p + 64, 0, 64);
  memset (p + 128, 0, 64);
  memset (p + 192, 0, 64);

  unsigned char *s = (unsigned char*) accept;
  /* Different from strcspn it does not add the NULL on the table
     so can avoid check if str[i] is NULL, since table['\0'] will
     be 0 and thus stopping the loop check.  */
  do
    p[*s++] = 1;
  while (*s);

  s = (unsigned char*) str;
  if (!p[s[0]]) return 0;
  if (!p[s[1]]) return 1;
  if (!p[s[2]]) return 2;
  if (!p[s[3]]) return 3;

  s = (unsigned char *) ((uintptr_t)(s) & ~3);
  unsigned int c0, c1, c2, c3;
  do {
      s += 4;
      c0 = p[s[0]];
      c1 = p[s[1]];
      c2 = p[s[2]];
      c3 = p[s[3]];
  } while ((c0 & c1 & c2 & c3) != 0);

  size_t count = s - (unsigned char *) str;
  return (c0 & c1) == 0 ? count + c0 : count + c2 + 2;
}
```
### strcspn
```c
/* Return the length of the maximum initial segment
   of S which contains only characters in ACCEPT.  */
size_t
STRSPN (const char *str, const char *accept)
{
  if (accept[0] == '\0')
    return 0;
  if (__glibc_unlikely (accept[1] == '\0'))
    {
      const char *a = str;
      for (; *str == *accept; str++);
      return str - a;
    }

  /* Use multiple small memsets to enable inlining on most targets.  */
  unsigned char table[256];
  unsigned char *p = memset (table, 0, 64);
  memset (p + 64, 0, 64);
  memset (p + 128, 0, 64);
  memset (p + 192, 0, 64);

  unsigned char *s = (unsigned char*) accept;
  /* Different from strcspn it does not add the NULL on the table
     so can avoid check if str[i] is NULL, since table['\0'] will
     be 0 and thus stopping the loop check.  */
  do
    p[*s++] = 1;
  while (*s);

  s = (unsigned char*) str;
  if (!p[s[0]]) return 0;
  if (!p[s[1]]) return 1;
  if (!p[s[2]]) return 2;
  if (!p[s[3]]) return 3;

  s = (unsigned char *) ((uintptr_t)(s) & ~3);
  unsigned int c0, c1, c2, c3;
  do {
      s += 4;
      c0 = p[s[0]];
      c1 = p[s[1]];
      c2 = p[s[2]];
      c3 = p[s[3]];
  } while ((c0 & c1 & c2 & c3) != 0);

  size_t count = s - (unsigned char *) str;
  return (c0 & c1) == 0 ? count + c0 : count + c2 + 2;
}
```

## The optimization
It's really not a crazy optimization, but I just noticed that, by calling both ```strspn``` and ```strcspn```, there is a redundancy in the use of two 256-byte tables. Greater levels of compiler optimization will correctly reduce the double 256-byte allocation to a single one, but the redundant instructions to reset and reinitalize the table with the delimiter string remain. So, along with some other minor optimizations, I manually inlined the two calls below.
```c
 /* Parse S into tokens separated by characters in DELIM.
 	x = strtok_r(NULL, "-=", &sp);	// x = "def", sp = NULL
 	x = strtok_r(NULL, "=", &sp);	// x = NULL
 		// s = "abc\0-def\0"

  This generic implementation can be thought of as 3 (sub-)routines:
    [0] Process delims - 
    Set up a look-up table with the delimiting characters for
    the input string to compare against
    [1] Find start - Iterate through input string until non-delimiting 
    character is reached-- basically strspn.
    [2] Find end - Iterate through input string until delimiting 
    character is reached-- basically strcspn.
 */
char *
__strtok_r (char *start, const char *delim, char **save_ptr)
 {
  /* General pointer used to cast START, DELIM, and *SAVE_PTR 
  as unsigned char pointers */
  unsigned char *u;
 
  /** BEGIN ROUTINE 0 **/
  /* Zero-initialize a character-indexed look-up table. The offsets
  corresponding to char values in DELIM store 1; otherwise, remain 0.
  See str(c)spn implementations for original reference. */
  unsigned char dset[256];
  memset (dset, 0, 64);
  memset (dset + 64, 0, 64);
  memset (dset + 128, 0, 64);
  memset (dset + 192, 0, 64);

  /* To fill the table, search for the NUL byte in DELIM by checking 4 bytes,
  and then aligning down (if, at all) to the closest lower
  4-byte boundary. Proceed to check 4 bytes at a time by loading into a
  4-byte integer 'word' */
  u = (unsigned char *)delim;

  if (__glibc_unlikely (u[0] == '\0')) ;
  else if (u[1] == '\0') dset[u[0]] = 1;
  else if (u[2] == '\0') dset[u[0]] = 1, dset[u[1]] = 1;
  else if (u[3] == '\0') dset[u[0]] = 1, dset[u[1]] = 1, dset[u[2]] = 1;
  else
     {
      dset[u[0]] = 1, dset[u[1]] = 1, dset[u[2]] = 1, dset[u[3]] = 1;
      
      /* Align down to 4-byte boundary (+ the 4 bytes already checked) */
      u = PTR_ALIGN_DOWN (u, 4) + 4;

#if __INT_LEAST32_WIDTH__ == 32 && __CHAR_BIT__ == 8

      uint_fast32_t zmask, word = *(uint_least32_t *)u;
      /* The classic bit-twiddling check for 0-byte in a word, in which
      the resulting 'mask', ZMASK, sets 0x80 where WORD contains the zero
      byte and 0x00 for nonzero bytes. Thus, break the loop if ZMASK
      isn't all zeros. */
      while ((zmask = ~word & (word - 0x01010101UL) & 0x80808080UL) == 0)
        {
          dset[u[0]] = 1;
          dset[u[1]] = 1;
          dset[u[2]] = 1;
          dset[u[3]] = 1;
          word = *(uint_least32_t *)(u += 4);
        }

/* macro to handle the remaining bytes using zmask */
# define handle_zmask(shft0, fst2_nonzero, shft2) \
  { \
    /* Move MSB to LSB and XOR to get (in bits):
      0..1 from 0..0
      0..0 from 1..0,
    effectively doing (!!) w/o flag dependency */ \
    dset[u[0]] = (unsigned char) (zmask >> shft0) ^ 1; \
    /* fst2_nonzero is true if the first 2 bytes of u are nonzero
    (i.e. zmask[0/1] = 0/0) */ \
    if (fst2_nonzero) \
      dset[u[1]] = 1, \
      dset[u[2]] = (unsigned char) (zmask >> shft2) ^ 1; \
  } 

/* handle the remaining bytes */
# if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  handle_zmask (7, (uint_least16_t)zmask == 0x0000, 23);
# elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  handle_zmask (31, zmask <= 0x8080, 15);
# elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
  handle_zmask (23, zmask <= 0x8080, 7);
# endif /* END __BYTE_ORDER__ == X */

#else  /* Portably do it in bytes */
    int c0 = (_Bool)u[0], 
        c1 = (_Bool)u[1], 
        c2 = (_Bool)u[2],
        c3 = (_Bool)u[3];

    while (c0 & c1 & c2 & c3)
      {
        dset[u[0]] = 1;
        dset[u[1]] = 1;
        dset[u[2]] = 1;
        dset[u[3]] = 1;

        u += 4;

        c0 = (_Bool)u[0];
        c1 = (_Bool)u[1]; 
        c2 = (_Bool)u[2];
        c3 = (_Bool)u[3];
      }
    
    dset[u[0]] = c0;
    if (c0 & c1) 
      dset[u[1]] = 1, 
      dset[u[2]] = c2;
#endif /* END __INT_LEAST32_WIDTH__ == 32 && __CHAR_BIT__ == 8 */
     }
  /** END ROUTINE 0 **/
 
  /* From this point on, U refers to the input string, conditionally
  START or *SAVE_P. */
  u = (unsigned char *)*save_ptr;
  if (__glibc_unlikely (start != NULL))
    u = (unsigned char *)start;

  /** BEGIN ROUTINE 1 **/
  /* Find first character in U that is not in DSET */
  if (!dset[u[0]])
    ;
  else if (!dset[u[1]])
    u += 1;
  else if (!dset[u[2]])
    u += 2;
  else if (!dset[u[3]])
    u += 3;
  else
    {
      /* If there were a 'fast type' for an implicit int (i.e. one without
      a specified minimum width), it should be that of the int with the
      minimum possible width, 16) */
      int_fast16_t s0, s2, det;

      u = PTR_ALIGN_DOWN (u, 4);
      do
        {
          u += 4;

          s0 = dset[u[0]];
          det = dset[u[1]] & s0;
          s2 = dset[u[2]];
        }
      while (det & s2 & dset[u[3]]);

      u += !det ? s0 : s2 + 2;
    }
  /** END ROUTINE 1 **/

  /* End of string is reached */
  if (__glibc_unlikely (*u == '\0'))
     {
      *save_ptr = (char *)u;
       return NULL;
     }
  /* End of string is not yet reached, so set START of return token */
  start = (char *)u;
  /* For NUL to continue causing a break in ROUTINE 2, set DSET[NUL] to 1 */
  dset['\0'] = 1;

  /** BEGIN ROUTINE 2 **/
  /* Find first character in start that is in DSET */
  if (dset[u[0]])
    ;
  else if (dset[u[1]])
    u += 1;
  else if (dset[u[2]])
    u += 2;
  else if (dset[u[3]])
    u += 3;
  else
    {
      int_fast16_t s0, s2, det;

      u = PTR_ALIGN_DOWN (u, 4);
      do
        {
          u += 4;

          s0  = dset[u[0]];
          det = dset[u[1]] | s0;
          s2  = dset[u[2]];
        }
      while ((det | s2 | dset[u[3]]) == 0);

      u += det ? 1 - s0 : 3 - s2;
    }
  /** END ROUTINE 2 **/

  *save_ptr = (char *)u;
  if (__glibc_likely (*u != 0))
     {
      *u = 0;
      (*save_ptr)++;
     }
  return start;
 }
```