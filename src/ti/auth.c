/*
 * auth.c
 */
#include <stdlib.h>
#include <ti/auth.h>
#include <ti.h>

#define TI__AUTH_REPR_SZ 6

/* allocate temporary buffer with enough space for storing an auth string */
static char ti__auth_str[512];

struct ti__auth_repr_s
{
    char * str;
    uint16_t mask;
};

static struct ti__auth_repr_s ti__auth_map[TI__AUTH_REPR_SZ] = {
        {.str="FULL",           .mask=TI_AUTH_MASK_FULL},
        {.str="QUERY",          .mask=TI_AUTH_QUERY},
        {.str="EVENT",          .mask=TI_AUTH_EVENT},
        {.str="GRANT",          .mask=TI_AUTH_GRANT},
        {.str="JOIN",           .mask=TI_AUTH_JOIN},
        {.str="RUN",            .mask=TI_AUTH_RUN},
};

ti_auth_t * ti_auth_create(ti_user_t * user, uint16_t mask)
{
    ti_auth_t * auth = malloc(sizeof(ti_auth_t));
    if (!auth)
        return NULL;
    auth->user = ti_grab(user);
    auth->mask = mask;
    return auth;
}

void ti_auth_destroy(ti_auth_t * auth)
{
    if (!auth)
        return;
    ti_user_drop(auth->user);
    free(auth);
}

const char * ti_auth_mask_to_str(uint16_t mask)
{
    char * pt = ti__auth_str;
    int i;
    struct ti__auth_repr_s * repr = ti__auth_map;

    for (i = 0; i < TI__AUTH_REPR_SZ && mask; i++, repr++)
    {
        if ((mask & repr->mask) == repr->mask)
        {
            mask -= repr->mask;
            pt += (pt == ti__auth_str)
                    ? sprintf(pt, "%s", repr->str)
                    : sprintf(pt, "|%s", repr->str);
        }
    }
    if (pt == ti__auth_str)
    {
        sprintf(pt, "NO_ACCESS");
    }
    return ti__auth_str;
}
