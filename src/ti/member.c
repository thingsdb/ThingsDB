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
    ti_member_t * member = ti_enum_member_by_strn(enum_, name->str, name->n);
    if (member)
    {
        ex_set(e, EX_VALUE_ERROR,
                "member `%s` on `%s` already exists"DOC_T_ENUM,
                member->name->str,
                enum_->name);
        return NULL;
    }

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
    member->tp = TI_VAL_MEMBER;

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

    ti_name_drop(member->name);
    ti_val_drop(member->val);

    free(member);
}

void ti_member_drop(ti_member_t * member)
{
    ti_name_t * name = member->name;
    ti_val_t * val = member->val;

    member->name = NULL;
    member->val = NULL;
    member->enum_ = NULL;

    ti_name_drop(name);
    ti_val_drop(val);

    if (!--member->ref)
        ti_member_destroy(member);
}

void ti_member_del(ti_member_t * member)
{
    ti_enum_del_member(member->enum_, member);
    ti_member_drop(member);
}

int ti_member_set_value(ti_member_t * member, ti_val_t * val, ex_t * e)
{
    if (ti_enum_check_val(member->enum_, val, e))
        return e->nr;

    ti_val_drop(member->val);
    member->val = val;
    ti_incref(val);

    return 0;
}

int ti_member_set_name(
        ti_member_t * member,
        const char * s,
        size_t n,
        ex_t * e)
{
    ti_name_t * name;

    if (ti_enum_member_by_strn(member->enum_, s, n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "member `%s` on `%s` already exists"DOC_T_ENUM,
                member->name->str,
                member->enum_->name);
        return e->nr;
    }

    name = ti_names_get(s, n);
    if (!name)
    {
        ex_set_mem(e);
        return e->nr;
    }

    ti_name_drop(member->name);
    member->name = name;

    return 0;
}
