#ifndef _WRAPPER_H
#define _WRAPPER_H

#include <sys/types.h>

#if defined(__sun)
typedef uint32_t u_int32_t;
#endif

typedef struct {
  u_int32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
} reg386_t;

extern void (*wrapper_target)(void);

extern int wrapper(void);
extern int null_call(void);

#endif /* _WRAPPER_H */

