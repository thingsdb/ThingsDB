/*
 * ti/mapping.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/mapping.h>

ti_mapping_t * ti_mapping_new(ti_field_t * t_field, ti_field_t * f_field)
{
    ti_mapping_t * mapping = malloc(sizeof(ti_mapping_t));
    if (!mapping)
        return NULL;

    mapping->t_field = t_field;
    mapping->f_field = f_field;
    return mapping;
}
