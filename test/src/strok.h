#ifndef _STROK_H

#define _GNU_SOURCE
#define _STROK_H
#include "print.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

/* 1 if 'type' is a pointer type, 0 otherwise.  */
#define __pointer_type(type) (__builtin_classify_type ((type)0) == 5)
/* intptr_t if P is true, or T if P is false.  */
#define __integer_if_pointer_type_sub(T, P)                                   \
  __typeof__(*(0 ? (__typeof__(0 ? (T *)0 : (void *)(P)))0                    \
                 : (__typeof__(0 ? (intptr_t *)0 : (void *)(!(P))))0))
/* intptr_t if EXPR has a pointer type, or the type of EXPR otherwise.  */
#define __integer_if_pointer_type(expr)                                       \
  __integer_if_pointer_type_sub (__typeof__((__typeof__(expr))0),             \
                                 __pointer_type (__typeof__(expr)))
/* Cast an integer or a pointer VAL to integer with proper type.  */
#define cast_to_integer(val) ((__integer_if_pointer_type (val)) (val))
/* Align a value by rounding down to closest size.
   e.g. Using size of 4096, we get this behavior:
        {4095, 4096, 4097} = {0, 4096, 4096}.  */
#define ALIGN_DOWN(base, size) ((base) & -((__typeof__(base)) (size)))
/* Align a value by rounding up to closest size.
   e.g. Using size of 4096, we get this behavior:
        {4095, 4096, 4097} = {4096, 4096, 8192}.
  Note: The size argument has side effects (expanded multiple times).  */
#define ALIGN_UP(base, size) ALIGN_DOWN ((base) + (size)-1, (size))
/* Same as ALIGN_DOWN(), but automatically casts when base is a pointer.  */
#define PTR_ALIGN_DOWN(base, size)                                            \
  ((__typeof__(base))ALIGN_DOWN ((uintptr_t) (base), (size)))
/* Same as ALIGN_UP(), but automatically casts when base is a pointer.  */
#define PTR_ALIGN_UP(base, size)                                              \
  ((__typeof__(base))ALIGN_UP ((uintptr_t) (base), (size)))

#ifndef BUF1PAGES
#define BUF1PAGES 1
#endif

#ifdef DEBUG
#define CHECKPOINT(cond) assert (cond);
#else
#define CHECKPOINT(cond) NULL;
#endif

char *strok_b (char *__restrict__, const char *__restrict__) __THROW
    __nonnull ((2));

char *strokr_b (char *__restrict__, const char *__restrict__, char **) __THROW
    __nonnull ((2, 3));

char *strok_a (char *__restrict__, const char *__restrict__) __THROW
    __nonnull ((2));

char *strokr_a (char *__restrict__, const char *__restrict__, char **) __THROW
    __nonnull ((2, 3));

char *strok_naive (char *__restrict__, const char *__restrict__) __THROW
    __nonnull ((2));

char *strokr_naive (char *__restrict__, const char *__restrict__,
                    char **) __THROW __nonnull ((2, 3));
#endif