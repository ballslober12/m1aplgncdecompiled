/*
 * lgnc_log.c
 *
 * All three functions implement their varargs forwarding in the original
 * MIPS o32 binary via a raw-register-spill trick (the incoming a1..a3 plus
 * overflow stack args are saved contiguously and the resulting pointer is
 * handed to printf()/fprintf() as if it continued that function's own
 * varargs). That trick is an o32-ABI-specific artifact, not portable C;
 * the behaviourally-identical, portable reconstruction is the standard
 * va_start/vprintf/vfprintf idiom used below.
 */
#include <stdarg.h>
#include "lgncopenapi.h"

/* LGNC_LOG_RAWPrint - .text:00013490 : unconditional printf passthrough */
int LGNC_LOG_RAWPrint(char *format, ...)
{
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = vprintf(format, ap);
    va_end(ap);
    return ret;
}

/* LGNC_LOG_DBGPrint - .text:000134d8 : printf passthrough gated on gLGNCDebugEnable */
int LGNC_LOG_DBGPrint(char *format, ...)
{
    int ret = 0;
    va_list ap;

    if (gLGNCDebugEnable == 1) {
        va_start(ap, format);
        ret = vprintf(format, ap);
        va_end(ap);
    }
    return ret;
}

/* LGNC_LOG_FDBGPrint - .text:00013548 : unconditional fprintf passthrough
 * (NB: despite the name, this one is NOT gated on gLGNCDebugEnable in the
 * original - only LGNC_LOG_DBGPrint is). */
int LGNC_LOG_FDBGPrint(FILE *stream, char *format, ...)
{
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = vfprintf(stream, format, ap);
    va_end(ap);
    return ret;
}
