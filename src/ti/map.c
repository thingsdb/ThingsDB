/*
 * ti/map.c
 */
#include <assert.h>
#include <stdlib.h>
#include <ti/map.h>
#include <ti/field.t.h>
#include <ti/mapping.h>

ti_map_t * ti_map_new(vec_t * mappings)
{
    ti_map_t * map = malloc(sizeof(ti_map_t));
    if (!map)
        return NULL;

    map->mappings = mappings;
    map->can_skip = false;
    for (vec_each(mappings, ti_mapping_t, mapping))
    {
        if (mapping->t_field->flags & TI_FIELD_FLAG_SKIP_NIL)
        {
            map->can_skip = true;
            break;
        }
    }
    return map;
}

void ti_map_destroy(ti_map_t * map)
{
    if (map)
        vec_destroy(map->mappings, free);
    free(map);
}


