/*
 * ti/verror.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/verror.h>
#include <util/strx.h>

ti_verror_t * ti_verror_create(const char * msg, size_t n, int8_t code)
{
    ti_verror_t * verror = malloc(sizeof(ti_verror_t) + n + 1);
    if (!verror)
        return NULL;
    assert (n <= EX_MAX_SZ);
    verror->ref = 1;
    verror->tp = TI_VAL_ERROR;
    verror->code = code;
    verror->msg_n = n;
    memcpy(verror->msg, msg, n);
    verror->msg[n] = '\0';
    return verror;
}

void ti_verror_to_e(ti_verror_t * verror, ex_t * e)
{
    e->n = verror->msg_n;
    e->nr = verror->code;
    memcpy(e->msg, verror->msg, e->n);
    e->msg[e->n] = '\0';
}

/* return 0 if valid, < 0 if not */
int ti_verror_check_msg(const char * msg, size_t n, ex_t * e)
{
    assert (e->nr == 0);
    if (n > EX_MAX_SZ)
        ex_set(e, EX_BAD_DATA,
                "error messages should not exceed %d characters, "
                "got %zu characters", EX_MAX_SZ, n);
    else if (!strx_is_utf8n(msg, n))
        ex_set(e, EX_BAD_DATA,
                "error messages must have valid UTF8 encoding");
    return e->nr;
}
