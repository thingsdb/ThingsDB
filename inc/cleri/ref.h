/*
 * ref.h - cleri ref element
 */
#ifndef CLERI_REF_H_
#define CLERI_REF_H_

#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>

/* typedefs */
typedef struct cleri_s cleri_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_ref(void);
void cleri_ref_set(cleri_t * ref, cleri_t * cl_obj);

#ifdef __cplusplus
}
#endif

#endif /* CLERI_REF_H_ */