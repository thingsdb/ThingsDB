/*
 * ti/warn.h
 */
#ifndef TI_WARN_H_
#define TI_WARN_H_

typedef enum
{
    TI_WARN_DEPRECATED_GET
} ti_warn_enum_t;

#include <ti/stream.h>

int ti_warn(ti_stream_t * stream, ti_warn_enum_t tp, const char * fmt, ...);

#endif  /* TI_WARN_H_ */
