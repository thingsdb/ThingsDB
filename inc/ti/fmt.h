/*
 * ti/fmt.h
 */
#ifndef TI_FMT_H_
#define TI_FMT_H_

typedef struct ti_fmt_s ti_fmt_t;

#include <util/buf.h>
#include <cleri/cleri.h>

struct ti_fmt_s
{
    buf_t buf;
    int indent_spaces;      /* number of spaces used for indentation */
    int indent;             /* indent level (step one) */
};


void ti_fmt_init(ti_fmt_t * fmt, int indent);
void ti_fmt_clear(ti_fmt_t * fmt);
int ti_fmt_nd(ti_fmt_t * fmt, cleri_node_t * nd);


#endif  /* TI_FMT_H_ */
