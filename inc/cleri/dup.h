/*
 * dup.h - this cleri element can be used to duplicate an element.
 */
#ifndef CLERI_DUP_H_
#define CLERI_DUP_H_

#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_dup_s cleri_dup_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_dup(uint32_t gid, cleri_t * cl_obj);

#ifdef __cplusplus
}
#endif

/* structs */
// cleri_dup_t is defined in cleri.h

#endif /* CLERI_DUP_H_ */