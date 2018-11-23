/*
 * ti/rpkg.c
 */

#include <ti/rpkg.h>
#include <stdlib.h>

ti_rpkg_t * ti_rpkg_create(ti_pkg_t * pkg)
{
    ti_rpkg_t * rpkg = malloc(sizeof(ti_rpkg_t));
    if (!rpkg)
        return NULL;
    rpkg->ref = 1;
    rpkg->pkg = pkg;
    return rpkg;
}

void ti_rpkg_drop(ti_rpkg_t * rpkg)
{
    if (rpkg && !--rpkg->ref)
    {
        free(rpkg->pkg);
        free(rpkg);
    }
}
