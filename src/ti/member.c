/*
 * ti/member.c
 */

#include <ti/member.h>
#include <doc.h>

/*
 * Increases the reference of `name` and `val` on success.
 *
 * The returned `member` has a borrowed reference, the `one` reference is
 * used by the parent `enum` type.
 */
ti_member_t * ti_member_create(
        ti_enum_t * enum_,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    ti_member_t * member;

    if (enum_->members->n == 0)
        ti_enum_set_enum_tp(enum_, val, e);
    else
        ti_enum_check_val(enum_, val, e);

    if (e->nr)
        return NULL;

    member = malloc(sizeof(ti_member_t));
    if (!member)
    {
        ex_set_mem(e);
        return NULL;
    }

    member->ref = 1;
    member->tp = TI_VAL_ENUM;

    member->name = name;
    member->val = val;
    member->enum_ = enum_;

    ti_incref(name);
    ti_incref(val);

    if (ti_enum_add_member(enum_, member, e))
    {
        ti_member_destroy(member);
        return NULL;
    }

    return member;
}

void ti_member_destroy(ti_member_t * member)
{
    if (!member)
        return;

    ti_val_drop(member->val);
    ti_name_drop(member->name);
    free(member);
}
