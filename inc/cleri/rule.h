/*
 * rule.h - cleri regular rule element. (do not directly use this element but
 *          create a 'prio' instead which will be wrapped by a rule element)
 */
#ifndef CLERI_RULE_H_
#define CLERI_RULE_H_

#include <stddef.h>
#include <inttypes.h>
#include <cleri/cleri.h>
#include <cleri/node.h>
#include <cleri/expecting.h>

/* typedefs */
typedef struct cleri_s cleri_t;
typedef struct cleri_node_s cleri_node_t;
typedef struct cleri_rule_tested_s cleri_rule_tested_t;
typedef struct cleri_rule_store_s cleri_rule_store_t;
typedef struct cleri_rule_s cleri_rule_t;

/* enums */
typedef enum cleri_rule_test_e
{
    CLERI_RULE_ERROR=-1,
    CLERI_RULE_FALSE,
    CLERI_RULE_TRUE
} cleri_rule_test_t;

/* private functions */
cleri_t * cleri__rule(uint32_t gid, cleri_t * cl_obj);
cleri_rule_test_t cleri__rule_init(
        cleri_rule_tested_t ** target,
        cleri_rule_tested_t * tested,
        const char * str);

/* structs */
struct cleri_rule_tested_s
{
    const char * str;
    cleri_node_t * node;
    cleri_rule_tested_t * next;
};

struct cleri_rule_store_s
{
    cleri_rule_tested_t tested;
    cleri_t * root_obj;
    size_t depth;
};

struct cleri_rule_s
{
    cleri_t * cl_obj;
};

#endif /* CLERI_RULE_H_ */