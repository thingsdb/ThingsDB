/*
 * ti/scope.h
 */
#ifndef TI_SCOPE_H_
#define TI_SCOPE_H_

typedef struct ti_scope_s ti_scope_t;

#include <ti/name.h>
#include <ti/thing.h>
#include <ti/val.h>

ti_scope_t * ti_scope_enter(ti_scope_t * scope, ti_thing_t * thing);
void ti_scope_leave(ti_scope_t ** scope, ti_scope_t * until);
ti_val_t * ti_scope_to_val(ti_scope_t * scope);
int ti_scope_push_name(ti_scope_t ** scope, ti_name_t * name, ti_val_t * val);
int ti_scope_push_thing(ti_scope_t ** scope, ti_thing_t * thing);
_Bool ti_scope_in_use_name(
        ti_scope_t * scope,
        ti_thing_t * thing,
        ti_name_t * name);

static inline _Bool ti_scope_is_thing(ti_scope_t * scope);

struct ti_scope_s
{
    ti_scope_t * prev;
    ti_thing_t * thing; /* with reference */
    ti_name_t * name;   /* weak reference to name or NULL */
    ti_val_t * val;     /* weak value, always the thing->name value */
};

static inline _Bool ti_scope_is_thing(ti_scope_t * scope)
{
    return scope->thing && !scope->name;
}

#endif  /* TI_SCOPE_H_ */
