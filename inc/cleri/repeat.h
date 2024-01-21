/*
 * repeat.h - cleri regular repeat element.
 */
#ifndef CLERI_REPEAT_H_
#define CLERI_REPEAT_H_

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_repeat_s cleri_repeat_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_repeat(uint32_t gid, cleri_t * cl_obj, size_t min, size_t max);

#ifdef __cplusplus
}
#endif

/* structs */
struct cleri_repeat_s
{
    cleri_t * cl_obj;
    size_t min;
    size_t max;
};

#endif /* CLERI_REPEAT_H_ */