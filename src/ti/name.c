/*
 * ti/name.c
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
    name->tp = TI_VAL_NAME;
    name->n = n;
    name->str[n] = '\0';
    return name;
}

/* call `ti_name_drop(..)`, not this function */
void ti_name_destroy(ti_name_t * name)
{
    (void) smap_pop(ti()->names, name->str);
    free(name);
}

_Bool ti_name_is_valid_strn(const char * str, size_t n)
{
    if (!n || (!isalpha(*str) && *str != '_'))
        return false;

    while(--n)
        if (!isalnum(str[n]) && str[n] != '_')
            return false;
    return true;
}
