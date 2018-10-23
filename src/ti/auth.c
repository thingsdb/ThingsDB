/*
 * auth.c
 */
#include <stdlib.h>
#include <ti/auth.h>

#define TI__AUTH_REPR_SZ 14

/* allocate temporary buffer with enough space for storing an auth string */
static char ti__auth_str[512];

struct ti__auth_repr_s
{
    char * str;
    uint16_t mask;
};

static struct ti__auth_repr_s ti__auth_map[TI__AUTH_REPR_SZ] = {
        /* profiles, biggest numbers must be fist */
        {.str="FULL",           .mask=TI_AUTH_MASK_FULL},
        {.str="MODIFY",         .mask=TI_AUTH_MASK_MODIFY},
        {.str="READ",           .mask=TI_AUTH_MASK_READ},

        /* flags, order here is not important */
        {.str="ACCESS",         .mask=TI_AUTH_ACCESS},
        {.str="WATCH",          .mask=TI_AUTH_WATCH},
        {.str="DB_CREATE",      .mask=TI_AUTH_DB_CREATE},
        {.str="DB_DROP",        .mask=TI_AUTH_DB_DROP},
        {.str="DB_RENAME",      .mask=TI_AUTH_DB_RENAME},
        {.str="USER_CREATE",    .mask=TI_AUTH_USER_CREATE},
        {.str="USER_DROP",      .mask=TI_AUTH_USER_DROP},
        {.str="USER_CHANGE",    .mask=TI_AUTH_USER_CHANGE},
        {.str="USER_OTHER",     .mask=TI_AUTH_USER_OTHER},
        {.str="GRANT",          .mask=TI_AUTH_GRANT},
        {.str="REVOKE",         .mask=TI_AUTH_REVOKE},
};

ti_auth_t * ti_auth_new(ti_user_t * user, uint16_t mask)
{
    ti_auth_t * auth = malloc(sizeof(ti_auth_t));
    if (!auth)
        return NULL;
    auth->user = user;
    auth->mask = mask;
    return auth;
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
