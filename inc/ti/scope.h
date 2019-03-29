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
#include <ti/prop.h>
#include <ti/ex.h>

ti_scope_t * ti_scope_enter(ti_scope_t * scope, ti_thing_t * thing);
void ti_scope_leave(ti_scope_t ** scope, ti_scope_t * until);
int ti_scope_push_name(ti_scope_t ** scope, ti_name_t * name, ti_val_t * val);
int ti_scope_push_thing(ti_scope_t ** scope, ti_thing_t * thing);
_Bool ti_scope_in_use_thing(ti_scope_t * scope, ti_thing_t * thing);
_Bool ti_scope_in_use_name(
        ti_scope_t * scope,
        ti_thing_t * thing,
        ti_name_t * name);
int ti_scope_local_from_node(ti_scope_t * scope, cleri_node_t * nd, ex_t * e);
ti_val_t *  ti_scope_find_local_val(ti_scope_t * scope, ti_name_t * name);
ti_val_t *  ti_scope_local_val(ti_scope_t * scope, ti_name_t * name);
int ti_scope_polute_prop(ti_scope_t * scope, ti_prop_t * prop);
int ti_scope_polute_val(ti_scope_t * scope, ti_val_t * val, int64_t idx);
static inline _Bool ti_scope_current_val_in_use(ti_scope_t * scope);
static inline _Bool ti_scope_is_thing(ti_scope_t * scope);
static inline _Bool ti_scope_is_raw(ti_scope_t * scope);
static inline _Bool ti_scope_has_local_name(
        ti_scope_t * scope,
        ti_name_t * name);
static inline ti_val_t * ti_scope_weak_get_val(ti_scope_t * scope);

struct ti_scope_s
{
    ti_scope_t * prev;
    ti_thing_t * thing; /* with reference */
    ti_name_t * name;   /* weak reference to name or NULL */
    ti_val_t * val;     /* weak reference to value */
    vec_t * local;      /* ti_prop_t (local props closure functions */
};


static inline ti_val_t * ti_scope_weak_get_val(ti_scope_t * scope)
{
    return scope->name ? scope->val : (ti_val_t *) scope->thing;
}

static inline _Bool ti_scope_is_thing(ti_scope_t * scope)
{
    assert (scope->thing);
    return !scope->name;
}

static inline _Bool ti_scope_is_raw(ti_scope_t * scope)
{
    return scope->name && ti_val_is_raw(scope->val);
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
