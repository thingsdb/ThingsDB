/*
 * expecting.c - holds elements which the grammar expects at one position.
 *               this can be used for suggestions.
 */
#include <cleri/expecting.h>
#include <stdlib.h>


static void expecting__empty(cleri_expecting_t * expecting);
static int expecting__get_mode(cleri_exp_modes_t * modes, const char * str);
static void expecting__shift_modes(
        cleri_exp_modes_t ** modes,
        const char * str);
static void expecting__modes_free(cleri_exp_modes_t * modes);

/*
 * Returns NULL in case an error has occurred.
 */
cleri_expecting_t * cleri__expecting_new(const char * str, int flags)
{
    cleri_expecting_t * expecting = cleri__malloc(cleri_expecting_t);

    if (expecting != NULL)
    {
        expecting->str = str;
        expecting->modes = NULL;

        if (flags & CLERI_FLAG_EXPECTING_DISABLED)
        {
            expecting->required = NULL;
            expecting->optional = NULL;
            return expecting;
        }

        if ((expecting->required = cleri__olist_new()) == NULL)
        {
            free(expecting);
            return NULL;
        }

        if ((expecting->optional = cleri__olist_new()) == NULL)
        {
            free(expecting->required);
            free(expecting);
            return NULL;
        }

    }

    return expecting;
}

/*
 * Returns 0 if the mode is set successful and -1 if an error has occurred.
 */
int cleri__expecting_update(
        cleri_expecting_t * expecting,
        cleri_t * cl_obj,
        const char * str)
{
    int rc = 0;
    if (expecting->required == NULL)
    {
        if (str > expecting->str)
        {
            expecting->str = str;
        }
        return 0;
    }


    if (str > expecting->str)
    {
        expecting__empty(expecting);
        expecting->str = str;
        expecting__shift_modes(&(expecting->modes), str);
    }

    if (expecting->str == str)
    {
        if (expecting__get_mode(expecting->modes, str))
        {
            /* true (1) is required */
            rc = cleri__olist_append_nref(expecting->required, cl_obj);
        }
        else
        {
            /* false (0) is optional */
            rc = cleri__olist_append_nref(expecting->optional, cl_obj);
        }
    }

    return rc;
}

/*
 * Returns 0 if the mode is set successful and -1 if an error has occurred.
 */
int cleri__expecting_set_mode(
        cleri_expecting_t * expecting,
        const char * str,
        int mode)
{
    cleri_exp_modes_t ** modes = &expecting->modes;

    if (expecting->required == NULL)
    {
        return 0;
    }

    for (; *modes != NULL; modes = &(*modes)->next)
    {
        if ((*modes)->str == str)
        {
            (*modes)->mode = mode && (*modes)->mode;
            return 0;
        }
    }

    *modes = cleri__malloc(cleri_exp_modes_t);

    if (*modes == NULL)
    {
        return -1;
    }

    (*modes)->mode = mode;
    (*modes)->next = NULL;
    (*modes)->str = str;

    return 0;
}

/*
 * Destroy expecting object.
 */
void cleri__expecting_free(cleri_expecting_t * expecting)
{
    expecting__empty(expecting);
    free(expecting->required);
    free(expecting->optional);
    expecting__modes_free(expecting->modes);
    free(expecting);
}

/*
 * append optional to required and sets optional to NULL
 */
void cleri__expecting_combine(cleri_expecting_t * expecting)
{
    cleri_olist_t * required = expecting->required;

    if (expecting->optional->cl_obj == NULL)
    {
        free(expecting->optional);
        expecting->optional = NULL;
    }

    if (required->cl_obj == NULL)
    {
        free(expecting->required);
        expecting->required = expecting->optional;
    }
    else
    {
        while (required->next != NULL)
        {
            required = required->next;
        }
        required->next = expecting->optional;
    }
    expecting->optional = NULL;
}

/*
 * shift from modes
 */
static void expecting__shift_modes(
        cleri_exp_modes_t ** modes,
        const char * str)
{
    cleri_exp_modes_t * next;

    while ((*modes)->next != NULL)
    {
        if ((*modes)->str == str)
        {
            break;
        }
        next = (*modes)->next;
        free(*modes);
        *modes = next;
    }
    (*modes)->str = str;
}

/*
 * Destroy modes.
 */
static void expecting__modes_free(cleri_exp_modes_t * modes)
{
    cleri_exp_modes_t * next;
    while (modes != NULL)
    {
        next = modes->next;
        free(modes);
        modes = next;
    }
}

/*
 * Return modes for a given position in str.
 */
static int expecting__get_mode(cleri_exp_modes_t * modes, const char * str)
{
    for (; modes != NULL; modes = modes->next)
    {
        if (modes->str == str)
        {
            return modes->mode;
        }
    }
    return CLERI__EXP_MODE_REQUIRED;
}

/*
 * Empty both required and optional lists.
 */
static void expecting__empty(cleri_expecting_t * expecting)
{
    cleri__olist_empty(expecting->required);
    cleri__olist_empty(expecting->optional);
}
