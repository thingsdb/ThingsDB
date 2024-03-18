/*
 * cleri.c - each cleri element is a cleri object.
 */
#include <cleri/cleri.h>
#include <stdlib.h>

static cleri_t end_of_statement = {
        .gid=0,
        .ref=1,
        .free_object=NULL,
        .parse_object=NULL,
        .tp=CLERI_TP_END_OF_STATEMENT,
        .via={.dummy=NULL}};
cleri_t * CLERI_END_OF_STATEMENT = &end_of_statement;

/*
 * Returns NULL in case an error has occurred.
 */
cleri_t * cleri_new(
        uint32_t gid,
        cleri_tp tp,
        cleri_free_object_t free_object,
        cleri_parse_object_t parse_object)
{
    cleri_t * cl_object = cleri__malloc(cleri_t);
    if (cl_object != NULL)
    {
        cl_object->gid = gid;
        cl_object->tp = tp;
        cl_object->ref = 1;
        cl_object->via.dummy = NULL;
        cl_object->free_object = free_object;
        cl_object->parse_object = parse_object;
    }
    return cl_object;
}

/*
 * Increment reference counter on cleri object.
 */
void cleri_incref(cleri_t * cl_object)
{
    cl_object->ref++;
}

/*
 * Decrement reference counter.
 * If no references are left the object is destoryed. (never the element)
 */
void cleri_decref(cleri_t * cl_object)
{
    if (!--cl_object->ref)
    {
        free(cl_object);
    }
}

/*
 * Recursive cleanup an object including the element.
 * If there are still references left, then only the element but not the object
 * is not destroyed and this function will return -1. If successful
 * cleaned the return value is 0.
 */
int cleri_free(cleri_t * cl_object)
{
    if (cl_object->tp == CLERI_TP_THIS)
    {
        return 0;
    }

    /* Use tp to check because we need to be sure this check validates false
     * before calling the free function. */
    if (cl_object->tp != CLERI_TP_REF)
    {
        /* Change the type so the other are treated as references */
        cl_object->tp = CLERI_TP_REF;

        (*cl_object->free_object)(cl_object);

        /* We decrement once more as soon as the element has joined at least
         * one other element so we don't have to run the cleanup on this
         * specific element. */
        if (cl_object->ref > 1)
        {
            cl_object->ref--;
        }
    }

    if (!--cl_object->ref)
    {
        free(cl_object);
        return 0;
    }
    return -1;
}



