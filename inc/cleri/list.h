/*
 * list.h - cleri list element.
 */
#ifndef CLERI_LIST_H_
#define CLERI_LIST_H_

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_list_s cleri_list_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_list(
        uint32_t gid,
        cleri_t * cl_obj,
        cleri_t * delimiter,
        size_t min,
        size_t max,
        int opt_closing);

#ifdef __cplusplus
}
#endif

/* structs */
struct cleri_list_s
{
    cleri_t * cl_obj;
    cleri_t * delimiter;
    size_t min;
    size_t max;
    int opt_closing;
};

#endif /* CLERI_LIST_H_ */