#ifndef XNU_HOST_SHIMS_H
#define XNU_HOST_SHIMS_H
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef unsigned long vm_offset_t;
typedef unsigned long vm_size_t;

#define kalloc(s) malloc(s)
#define kfree(p, s) free(p)

#define panic(...) do { fprintf(stderr, "PANIC: " __VA_ARGS__); abort(); } while(0)

/* os/overflow.h from XNU provides os_add3_overflow */

/* Mach types minimal */
typedef void *task_t;
typedef void *thread_t;

#endif
