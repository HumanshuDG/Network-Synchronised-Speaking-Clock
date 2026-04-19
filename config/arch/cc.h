/*
 * cc.h
 * lwIP architecture/compiler definitions for ARM Cortex-M3 (GCC)
 */

#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Compiler / Platform
 * --------------------------------------------------------------------------- */
#define BYTE_ORDER  LITTLE_ENDIAN

/* Use the compiler's built-in byte swap if available */
#ifndef LWIP_PLATFORM_BYTESWAP
#define LWIP_PLATFORM_BYTESWAP  0
#endif

/* ---------------------------------------------------------------------------
 * Diagnostics / Debug Output
 * --------------------------------------------------------------------------- */
extern void uart_puts(const char *s);

/* Printf formatting for lwIP types */
#define U16_F   "hu"
#define S16_F   "hd"
#define X16_F   "hx"
#define U32_F   "u"
#define S32_F   "d"
#define X32_F   "x"
#define SZT_F   "u"

/* Platform-level debug output */
#define LWIP_PLATFORM_DIAG(x) do { uart_printf x; } while(0)
#define LWIP_PLATFORM_ASSERT(x)                         \
    do {                                                \
        uart_puts("lwIP ASSERT: ");                     \
        uart_puts(x);                                   \
        uart_puts("\r\n");                               \
        while(1);                                        \
    } while(0)

/* ---------------------------------------------------------------------------
 * Structure packing
 * --------------------------------------------------------------------------- */
#define PACK_STRUCT_FIELD(x)    x
#define PACK_STRUCT_STRUCT      __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* ---------------------------------------------------------------------------
 * Critical sections (no-op in NO_SYS mode with single-threaded polling)
 * SYS_ARCH_DECL_PROTECT must declare a variable that the other macros use.
 * --------------------------------------------------------------------------- */
#define SYS_ARCH_DECL_PROTECT(lev)  unsigned int lev
#define SYS_ARCH_PROTECT(lev)       do { lev = 0; (void)lev; } while(0)
#define SYS_ARCH_UNPROTECT(lev)     do { (void)lev; } while(0)

/* Provide a declaration for the printf helper used in LWIP_PLATFORM_DIAG */
extern int uart_printf(const char *fmt, ...);

#endif /* LWIP_ARCH_CC_H */
