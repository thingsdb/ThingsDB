/*
 * name.c
 */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <ti/name.h>
#include <ti/val.h>
#include <ti.h>
#include <util/logger.h>

ti_name_t * ti_name_create(const char * str, size_t n)
{
    ti_name_t * name = malloc(sizeof(ti_name_t) + n + 1);
    if (!name)
        return NULL;

    memcpy(name->str, str, n);
    name->ref = 1;
    name->tp = TI_VAL_RAW;
    name->n = n;
    name->str[n] = '\0';
    return name;
}

void ti_name_drop(ti_name_t * name)
{
    if (name && !--name->ref)
    {
        (void *) smap_pop(ti()->names, name->str);
        ti_name_destroy(name);
    }
}

_Bool ti_name_is_valid_strn(const char * str, size_t n)
{
    if (!n || (!isalpha(str[0]) && str[0] != '_'))
        return false;

    while(--n)
    {
        if (!isalnum(str[n]) && str[n] != '_')
            return false;
    }
    return true;
}
