#ifndef _PRINT_H
#define _PRINT_H
#include <stdio.h>

#define FMT(x)                                                                \
  _Generic((x), \
    char: "%c", \
    signed char: "%hhd", \
    unsigned char: "%hhu", \
    signed short: "%hd", \
    unsigned short: "%hu", \
    signed int: "%d", \
    unsigned int: "%u", \
    long int: "%ld", \
    unsigned long int: "%lu", \
    long long int: "%lld", \
    unsigned long long int: "%llu", \
    float: "%f", \
    double: "%f", \
    long double: "%Lf", \
    char *: "%s", \
    void *: "%p")

#define PRINT(fmt, ...) printf (fmt, __VA_ARGS__)
#define PVAR(var) printf (#var "="), printf (FMT (var), var), printf ("\n")
#define TOKEN(tok) #tok
#define STRING(...) __VA_ARGS__

static void __attribute__ ((unused))
prints_hex (const char *e, const char *a)
{
  printf ("Expected: ");

  do
    {
      printf ("0x%hx ", (char)*e++);
    }
  while (*e++);
  printf ("\n");
  printf ("Actual: ");
  
    {
      printf ("0x%hx ", (char)*a++);
    }
    while (*a++)
  printf ("\n\n");
}
#endif