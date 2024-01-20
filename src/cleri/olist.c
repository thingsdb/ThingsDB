/*
 * olist.c - linked list for keeping cleri objects.
 */
#include <cleri/olist.h>
#include <stdlib.h>

/*
 * Returns NULL in case an error has occurred.
 */
cleri_olist_t * cleri__olist_new(void)
{
    cleri_olist_t * olist = cleri__malloc(cleri_olist_t);
    if (olist != NULL)
    {
        olist->cl_obj = NULL;
        olist->next = NULL;
    }
    return olist;
}

/*
 * Returns 0 if successful or -1 in case of an error.
 * The list remains unchanged in case of an error and the object reference
 * counter will be incremented when successful.
 */
int cleri__olist_append(cleri_olist_t * olist, cleri_t * cl_object)
{
    if (cl_object == NULL)
    {
        return -1;
    }

    if (olist->cl_obj == NULL)
    {
        cleri_incref(cl_object);
        olist->cl_obj = cl_object;
        olist->next = NULL;
        return 0;
    }

    while (olist->next != NULL)
    {
        olist = olist->next;
    }

    olist->next = cleri__malloc(cleri_olist_t);

    if (olist->next == NULL)
    {
        return -1;
    }

    cleri_incref(cl_object);
    olist->next->cl_obj = cl_object;
    olist->next->next = NULL;

    return 0;
}

/*
 * Exactly the same as cleri__olist_append, except the reference counter
 * will not be incremented.
 */
int cleri__olist_append_nref(cleri_olist_t * olist, cleri_t * cl_object)
{
    if (cl_object == NULL)
    {
        return -1;
    }

    if (olist->cl_obj == NULL)
    {
        olist->cl_obj = cl_object;
        olist->next = NULL;
        return 0;
    }

    while (olist->next != NULL)
    {
        olist = olist->next;
    }

    olist->next = cleri__malloc(cleri_olist_t);

    if (olist->next == NULL)
    {
        return -1;
    }

    olist->next->cl_obj = cl_object;
    olist->next->next = NULL;

    return 0;
}

/*
 * Destroy the olist and decrement the reference counter for each object in
 * the list. (NULL is allowed as olist and does nothing)
 */
void cleri__olist_free(cleri_olist_t * olist)
{
    cleri_olist_t * next;
    while (olist != NULL)
    {
        next = olist->next;
        cleri_free(olist->cl_obj);
        free(olist);
        olist = next;
    }
}


/*
 * Empty the object list but do not free the list. This will not decrement
 * the reference counters for object in the list.
 */
void cleri__olist_empty(cleri_olist_t * olist)
{
    cleri_olist_t * current;

    if (olist == NULL)
    {
        return;
    }

    /* set root object to NULL but we do not need to free root */
    olist->cl_obj = NULL;

    /* set root next to NULL */
    current = olist->next;
    olist->next = NULL;

    /* cleanup all the rest */
    while (current != NULL)
    {
        olist = current->next;
        free(current);
        current = olist;
    }
}

/*
 * Cancel building olist. Used to recover from an error while creating
 * a new element.
 */
void cleri__olist_cancel(cleri_olist_t * olist)
{
    cleri_olist_t * current = olist->next;

    /* cleanup all the rest */
    while (current != NULL)
    {
        olist->cl_obj->ref--;
        current = current->next;
    }
    cleri__olist_empty(olist);
}

void cleri__olist_unique(cleri_olist_t * olist)
{
    while (olist != NULL && olist->next != NULL)
    {
        cleri_olist_t * test = olist;
        while (test->next != NULL)
        {
            if (olist->cl_obj == test->next->cl_obj)
            {
                cleri_olist_t * tmp = test->next->next;
                free(test->next);
                test->next = tmp;
                continue;
            }
            test = test->next;
        }
        olist = olist->next;
    }
}