/*
 * choice.h - this cleri element can hold other elements and the grammar
 *            has to choose one of them.
 */
#ifndef CLERI_CHOICE_H_
#define CLERI_CHOICE_H_

#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>
#include <cleri/olist.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_olist_s cleri_olist_t;
typedef struct cleri_choice_s cleri_choice_t;

/* public functions */
#ifdef __cplusplus
extern "C" {
#endif

cleri_t * cleri_choice(uint32_t gid, int most_greedy, size_t len, ...);

#ifdef __cplusplus
}
#endif

/* structs */
struct cleri_choice_s
{
    int most_greedy;
    cleri_olist_t * olist;
};

#endif /* CLERI_CHOICE_H_ */