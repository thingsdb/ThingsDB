/*
 * ti/map.h
 */
#ifndef TI_MAP_H_
#define TI_MAP_H_

typedef struct ti_map_s ti_map_t;

#include <util/vec.h>
#include <stdbool.h>

ti_map_t * ti_map_new(vec_t * mappings);
void ti_map_destroy(ti_map_t * map);

struct ti_map_s
{
    vec_t * mappings;  /* ti_mapping_t */
    bool can_skip;   /* true when size might be less */
};

#endif  /* TI_MAP_H_ */
