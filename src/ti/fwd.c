/*
 * ti/fwd.c
 */
#include <stdlib.h>
#include <ti.h>
#include <ti/fwd.h>


ti_fwd_t * ti_fwd_create(uint16_t orig_pkg_id, ti_stream_t * src_stream)
{
    ti_fwd_t * fwd = malloc(sizeof(ti_fwd_t));
    if (!fwd)
        return NULL;
    fwd->stream = ti_grab(src_stream);
    fwd->orig_pkg_id = orig_pkg_id;
    return fwd;
}

void ti_fwd_destroy(ti_fwd_t * fwd)
{
    if (!fwd)
        return;
    ti_stream_drop(fwd->stream);
    free(fwd);
}
