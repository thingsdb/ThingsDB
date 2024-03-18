/*
 * optional.h - cleri optional element.
 */
#ifndef CLERI_OPTIONAL_H_
#define CLERI_OPTIONAL_H_

#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_optional_s cleri_optional_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_optional(uint32_t gid, cleri_t * cl_obj);

#ifdef __cplusplus
}
#endif

/* structs */
struct cleri_optional_s
{
    cleri_t * cl_obj;
};

#endif /* CLERI_OPTIONAL_H_ */