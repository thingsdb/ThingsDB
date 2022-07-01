/*
 * ti/fmt.h
 */
#ifndef TI_FMT_H_
#define TI_FMT_H_

typedef struct ti_fmt_s ti_fmt_t;

#include <util/buf.h>
#include <cleri/cleri.h>
#include <ti/raw.t.h>

/* When 0, use TAB, else the number of spaces */
#define TI_FMT_SPACES 0

struct ti_fmt_s
{
    buf_t buf;
    int spaces;             /* when 0, use TAB, else spaces */
    int indent;             /* indent level (step one) */
};

void ti_fmt_init(ti_fmt_t * fmt, int spaces);
void ti_fmt_clear(ti_fmt_t * fmt);
int ti_fmt_nd(ti_fmt_t * fmt, cleri_node_t * nd);
int ti_fmt_ti_string(ti_fmt_t * fmt, ti_raw_t * raw);

static inline int ti_fmt_indent(ti_fmt_t * fmt)
{
    if (fmt->spaces)
    {
        for (int i = fmt->indent * fmt->spaces; i; --i)
            if (buf_write(&fmt->buf, ' '))
                return -1;
    }
    else
    {
        for (int i = fmt->indent; i; --i)
            if (buf_write(&fmt->buf, '\t'))
                return -1;
    }
    return 0;
}

#endif  /* TI_FMT_H_ */
