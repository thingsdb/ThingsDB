/*
 * ti/epkg.c
 */

#include <ti/epkg.h>
#include <stdlib.h>

ti_epkg_t * ti_epkg_create(ti_pkg_t * pkg, uint64_t event_id)
{
    ti_epkg_t * epkg = malloc(sizeof(ti_epkg_t));
    if (!epkg)
        return NULL;
    epkg->ref = 1;
    epkg->pkg = pkg;
    epkg->event_id = event_id;
    return epkg;
}
