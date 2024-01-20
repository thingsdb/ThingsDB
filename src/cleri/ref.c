/*
 * ref.c - cleri ref element.
 */
#include <stdlib.h>
#include <assert.h>
#include <cleri/ref.h>

static void ref__free(cleri_t * cl_object);

/*
 * Returns NULL in case an error has occurred.
 */
cleri_t * cleri_ref(void)
{
    cleri_t * cl_object = cleri_new(
            0,
            CLERI_TP_REF,
            &ref__free,
            NULL);

    return cl_object;
}

/*
 * Both ref and cl_obj are not allowed to be NULL.
 */
void cleri_ref_set(cleri_t * ref, cleri_t * cl_obj)
{
    /* cl_obj should have no other refs */
    assert (ref != NULL &&
            ref->tp == CLERI_TP_REF &&
            cl_obj != NULL &&
            cl_obj->ref == 1);

    /* assign properties, except ref counter */
    ref->gid = cl_obj->gid;
    ref->free_object = cl_obj->free_object;
    ref->parse_object = cl_obj->parse_object;
    ref->tp = cl_obj->tp;
    ref->via = cl_obj->via;

    /* free *cl_obj and set the pointer to the ref object */
    free(cl_obj);
}


/*
 * Destroy ref object. (only used when ref is not set)
 */
static void ref__free(cleri_t * cl_object __attribute__((unused)))
{
    /* nothing todo */
}
