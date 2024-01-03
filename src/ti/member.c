/*
 * ti/member.c
 */
#include <doc.h>
#include <ti/enum.h>
#include <ti/enum.inline.h>
#include <ti/member.h>
#include <ti/names.h>
#include <ti/val.inline.h>

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

    if (ti_enum_member_by_strn(enum_, name->str, name->n) ||
        ti_enum_get_method(enum_, name))
    {
        ex_set(e, EX_VALUE_ERROR,
            "member or method `%s` already exists on enum `%s`"DOC_T_TYPED,
            name->str,
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

ti_member_t * ti_member_placeholder(ti_enum_t * enum_, ex_t * e)
{
    int r;
    static char buf[8];
    ti_member_t * member = malloc(sizeof(ti_member_t));
    if (!member)
        goto alloc_error;

    member->ref = 1;
    member->tp = TI_VAL_MEMBER;
    member->name = NULL;
    member->val = NULL;

    /* max 65536 thus M65536 */
    r = snprintf(buf, 8, "M%"PRIu32, enum_->members->n);
    if (r <= 1 || r > 7)
        goto alloc_error;

    member->name = ti_names_get(buf, (size_t) r);
    member->val = (ti_val_t *) ti_vint_create(enum_->members->n);

    if (!member->val || !member->name)
        goto alloc_error;

    if (ti_enum_add_member(enum_, member, e))
        goto failed;

    return member;

alloc_error:
    ex_set_mem(e);
failed:
    ti_member_destroy(member);
    return NULL;
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
    if (member && !--member->ref)
        ti_member_destroy(member);
}

void ti_member_remove(ti_member_t * member)
{
    ti_name_t * name = member->name;
    ti_val_t * val = member->val;

    member->name = NULL;
    member->val = NULL;
    member->enum_ = NULL;

    ti_name_drop(name);
    ti_val_drop(val);

    ti_member_drop(member);
}

void ti_member_del(ti_member_t * member)
{
    ti_enum_del_member(member->enum_, member);
    ti_member_remove(member);
}

void ti_member_def(ti_member_t * member)
{
    ti_member_t * tmp = VEC_first(member->enum_->members);
    tmp->idx = member->idx;
    member->idx = 0;
    vec_swap(member->enum_->members, member->idx, tmp->idx);
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

    if (!ti_name_is_valid_strn(s, n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "member name must follow the naming rules"DOC_NAMES);
        return e->nr;
    }

    name = ti_names_get(s, n);
    if (!name)
    {
        ex_set_mem(e);
        return e->nr;
    }

    if (ti_enum_get_method(member->enum_, name))
        goto fail_exists;

    switch(smap_add(member->enum_->smap, name->str, member))
    {
    case SMAP_ERR_ALLOC:
        ex_set_mem(e);
        goto fail0;
    case SMAP_ERR_EXIST:
        goto fail_exists;
    }

    (void) smap_pop(member->enum_->smap, member->name->str);
    ti_name_drop(member->name);
    member->name = name;

    return 0;

fail_exists:
    ex_set(e, EX_VALUE_ERROR,
        "member or method `%s` already exists on enum `%s`"DOC_T_TYPED,
        name->str,
        member->enum_->name);
fail0:
    ti_name_drop(name);
    return e->nr;

}
