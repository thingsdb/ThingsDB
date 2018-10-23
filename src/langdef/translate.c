/*
 * langdef/translate.h
 */
#include <langdef/translate.h>
#include <langdef/langdef.h>

const char * langdef_translate(cleri_t * elem)
{
    switch (elem->gid)
    {
    case CLERI_GID_STRING:
        return "<string>";
    case CLERI_GID_IDENTIFIER:
        return "<identifier>";
    case CLERI_GID_ARRAY_IDX:
        return "<index>";
    }
    return NULL;
}
