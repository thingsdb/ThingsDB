/*
 * ti/scope.h
 */
#ifndef TI_SCOPE_H_
#define TI_SCOPE_H_

typedef struct ti_scope_s ti_scope_t;

#include <cleri/cleri.h>
#include <ti/name.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <ti/ex.h>

ti_scope_t * ti_scope_enter(ti_scope_t * scope, ti_thing_t * thing);
void ti_scope_leave(ti_scope_t ** scope, ti_scope_t * until);
ti_val_t * ti_scope_global_to_val(ti_scope_t * scope);
int ti_scope_push_name(ti_scope_t ** scope, ti_name_t * name, ti_val_t * val);
int ti_scope_push_thing(ti_scope_t ** scope, ti_thing_t * thing);
_Bool ti_scope_in_use_name(
        ti_scope_t * scope,
        ti_thing_t * thing,
        ti_name_t * name);
int ti_scope_local_from_arrow(ti_scope_t * scope, cleri_node_t * nd, ex_t * e);
ti_val_t *  ti_scope_find_local_val(ti_scope_t * scope, ti_name_t * name);
ti_val_t *  ti_scope_local_val(ti_scope_t * scope, ti_name_t * name);
static inline _Bool ti_scope_current_val_in_use(ti_scope_t * scope);
static inline _Bool ti_scope_is_thing(ti_scope_t * scope);
static inline _Bool ti_scope_has_local_name(
        ti_scope_t * scope,
        ti_name_t * name);

struct ti_scope_s
{
    ti_scope_t * prev;
    ti_thing_t * thing; /* with reference */
    ti_name_t * name;   /* weak reference to name or NULL */
    ti_val_t * val;     /* weak value, always the thing->name value */
    vec_t * local;      /* ti_prop_t (local props arrow functions */
};

static inline _Bool ti_scope_is_thing(ti_scope_t * scope)
{
    return scope->thing && !scope->name;
}

static inline _Bool ti_scope_current_val_in_use(ti_scope_t * scope)
{
    return !scope->val
            ? false
            : ti_scope_in_use_name(scope->prev, scope->thing, scope->name);
}

/* only in the current scope */
static inline _Bool ti_scope_has_local_name(
        ti_scope_t * scope,
        ti_name_t * name)
{
    return !!ti_scope_local_val(scope, name);
}

#endif  /* TI_SCOPE_H_ */
