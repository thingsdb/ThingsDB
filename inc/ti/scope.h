/*
 * ti/scope.h
 */
#ifndef TI_SCOPE_H_
#define TI_SCOPE_H_

typedef struct ti_scope_s ti_scope_t;

#include <cleri/cleri.h>
#include <ti/closure.h>
#include <ti/ex.h>
#include <ti/name.h>
#include <ti/prop.h>
#include <ti/thing.h>
#include <ti/val.h>

ti_scope_t * ti_scope_enter(ti_scope_t * scope, ti_thing_t * thing);
void ti_scope_leave(ti_scope_t ** scope, ti_scope_t * until);

_Bool ti_scope_in_use_thing(ti_scope_t * scope, ti_thing_t * thing);
_Bool ti_scope_in_use_name(
        ti_scope_t * scope,
        ti_thing_t * thing,
        ti_name_t * name);
_Bool ti_scope_in_use_val(ti_scope_t * scope, ti_val_t * val);
int ti_scope_local_from_closure(
        ti_scope_t * scope,
        ti_closure_t * closure,
        ex_t * e);
ti_val_t *  ti_scope_find_local_val(ti_scope_t * scope, ti_name_t * name);
ti_val_t *  ti_scope_local_val(ti_scope_t * scope, ti_name_t * name);
int ti_scope_polute_prop(ti_scope_t * scope, ti_prop_t * prop);
int ti_scope_polute_val(ti_scope_t * scope, ti_val_t * val, int64_t idx);
static inline void ti_scope_set_name_val(
        ti_scope_t * scope,
        ti_name_t * name,
        ti_val_t * val);
static inline _Bool ti_scope_has_local_name(
        ti_scope_t * scope,
        ti_name_t * name);
static inline _Bool ti_scope_is_attached(ti_scope_t * scope);

struct ti_scope_s
{
    ti_scope_t * prev;
    ti_thing_t * thing; /* with reference */
    ti_name_t * name;   /* weak reference to name or NULL */
    ti_val_t * val;     /* weak reference to val or NULL */
    vec_t * local;      /* ti_prop_t (local props closure functions */
};

static inline void ti_scope_set_name_val(
        ti_scope_t * scope,
        ti_name_t * name,
        ti_val_t * val)
{
    assert (scope->name == NULL);
    assert (scope->val == NULL);

    scope->name = name;
    scope->val = val;
}

/* only in the current scope */
static inline _Bool ti_scope_has_local_name(
        ti_scope_t * scope,
        ti_name_t * name)
{
    return !!ti_scope_local_val(scope, name);
}

static inline _Bool ti_scope_is_attached(ti_scope_t * scope)
{
    return scope->thing->id && scope->name;
}
#endif  /* TI_SCOPE_H_ */
