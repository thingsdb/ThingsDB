/*
 * langdef/translate.c
 */
#include <langdef/translate.h>
#include <langdef/langdef.h>

const char * langdef_translate(cleri_t * elem)
{
    switch (elem->gid)
    {
    case CLERI_GID_T_STRING:
        return "<string>";
    case CLERI_GID_T_INT:
        return "<int>";
    case CLERI_GID_IDENTIFIER:
        return "<identifier>";
    }
    return NULL;
}
