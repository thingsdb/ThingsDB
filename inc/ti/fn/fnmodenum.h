#include <ti/fn/fn.h>

static void enum__add(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    static const char * fnname = "mod_enum` with task `add";
    cleri_node_t * child;
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_ENUM_ADD, 4, nargs, e))
        return;

    child = nd->children->next->next->next->next->next->next;

    if (ti_do_statement(query, child, e))
        return;

    if (ti_val_is_closure(query->rval))
    {
        if (ti_enum_add_method(enum_, name, (ti_closure_t *) query->rval, e))
            return;
    }
    else if (!ti_member_create(enum_, name, query->rval, e))
        return;

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
        goto panic;

    /* update modified time-stamp */
    enum_->modified_at = util_now_usec();

    /* query->rval might be null; when there are no instances */
    if (ti_task_add_mod_enum_add(task, enum_, name, query->rval))
        goto panic;

    return;  /* success */
panic:
    ex_set_mem(e);
    ti_panic("failed to get enum task");
}

static void enum__def(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);

    static const char * fnname = "mod_enum` with task `def";
    ti_member_t * member = ti_enum_member_by_strn(enum_, name->str, name->n);
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_ENUM_DEF, 3, nargs, e))
        return;

    if (!member)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member `%s`",
                enum_->name, name->str);
        return;
    }

    if (!member->idx)
        return;  /* already set as default */

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        return;
    }

    /* update modified time-stamp */
    enum_->modified_at = util_now_usec();

    if (ti_task_add_mod_enum_def(task, member))
    {
        ex_set_mem(e);
        return;
    }

    ti_member_def(member);
}


static void enum__del(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    static const char * fnname = "mod_enum` with task `del";
    ti_member_t * member = ti_enum_member_by_strn(enum_, name->str, name->n);
    ti_method_t * method = member ? NULL : ti_enum_get_method(enum_, name);
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_ENUM_DEL, 3, nargs, e))
        return;

    if (!member && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member or method `%s`",
                enum_->name, name->str);
        return;
    }

    if (member)
    {
        if (enum_->members->n == 1)
        {
            ex_set(e, EX_LOOKUP_ERROR,
                    "cannot delete `%s{%s}` as this is the last enum member",
                    enum_->name, name->str);
            return;
        }

        if (member->ref > 1)
        {
            ex_set(e, EX_OPERATION,
                    "enum member `%s{%s}` is still in use",
                    enum_->name, name->str);
            return;
        }
    }

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        return;
    }

    /* update modified time-stamp */
    enum_->modified_at = util_now_usec();

    if (ti_task_add_mod_enum_del(task, enum_, name))
        ex_set_mem(e);

    if (member)
        ti_member_del(member);
    else
        ti_enum_remove_method(enum_, name);
}

static void enum__mod(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    static const char * fnname = "mod_enum` with task `mod";
    cleri_node_t * child;
    ti_member_t * member = ti_enum_member_by_strn(enum_, name->str, name->n);
    ti_method_t * method = member ? NULL : ti_enum_get_method(enum_, name);
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_ENUM_MOD, 4, nargs, e))
        return;

    child = nd->children->next->next->next->next->next->next;

    if (!member && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member or method `%s`",
                enum_->name, name->str);
        return;
    }

    if (ti_do_statement(query, child, e))
        return;

    if (ti_opr_eq(member->val, query->rval))
        return;  /* do nothing, values are equal */

    if (ti_val_is_closure(query->rval))
    {
        if (!method)
        {
            ex_set(e, EX_TYPE_ERROR,
                "cannot convert a member into a method"DOC_MOD_ENUM_MOD);
            return;
        }
        ti_method_set_closure(method, (ti_closure_t *) query->rval);
    }
    else
    {
        if (!member)
        {
            ex_set(e, EX_TYPE_ERROR,
                "cannot convert a method into a member"DOC_MOD_ENUM_MOD);
            return;
        }
        if (ti_member_set_value(member, query->rval, e))
            return;
    }

    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        return;
    }

    /* update modified time-stamp */
    enum_->modified_at = util_now_usec();

    if (ti_task_add_mod_enum_mod(task, enum_, name, query->rval))
        ex_set_mem(e);
}

static void enum__ren(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    static const char * fnname = "mod_enum` with task `ren";
    cleri_node_t * child;
    ti_member_t * member = ti_enum_member_by_strn(enum_, name->str, name->n);
    ti_method_t * method = member ? NULL : ti_enum_get_method(enum_, name);
    ti_task_t * task;
    ti_raw_t * rname;
    ti_name_t * to;

    if (fn_nargs(fnname, DOC_MOD_ENUM_REN, 4, nargs, e))
        return;

    child = nd->children->next->next->next->next->next->next;

    if (!member && !method)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member or method `%s`",
                enum_->name, name->str);
        return;
    }

    if (ti_do_statement(query, child, e) ||
        fn_arg_str_slow(fnname, DOC_MOD_ENUM_REN, 4, query->rval, e))
        return;

    if (ti_opr_eq((ti_val_t *) name, query->rval))
        return;  /* do nothing, name is equal to current name */

    rname = (ti_raw_t *) query->rval;

    if (member)
    {
        if (ti_member_set_name(member, (const char *) rname->data, rname->n, e))
            return;
    }
    else if (ti_method_set_name_e(
            method,
            enum_,
            (const char *) rname->data,
            rname->n,
            e))
        return;

    to = member ? member->name : method->name;
    task = ti_task_get_task(query->change, query->collection->root);
    if (!task)
    {
        ex_set_mem(e);
        return;
    }

    /* update modified time-stamp */
    enum_->modified_at = util_now_usec();

    if (ti_task_add_mod_enum_ren(task, enum_, name, to))
        ex_set_mem(e);
}

static int do__f_mod_enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_enum_t * enum_;
    ti_name_t * name;
    ti_raw_t * rmod;
    cleri_node_t * child = nd->children;
    const int nargs = fn_get_nargs(nd);

    if (fn_not_collection_scope("mod_enum", query, e) ||
        fn_nargs_min("mod_enum", DOC_MOD_ENUM, 3, nargs, e) ||
        ti_do_statement(query, child, e) ||
        fn_arg_str_slow("mod_enum", DOC_MOD_ENUM, 1, query->rval, e))
        return e->nr;

    enum_ = ti_enums_by_raw(query->collection->enums, (ti_raw_t *) query->rval);
    if (!enum_)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "enum", e);

    if (ti_enum_try_lock(enum_, e))
        return e->nr;

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next), e) ||
        fn_arg_name_check("mod_enum", DOC_MOD_ENUM, 2, query->rval, e))
        goto fail0;

    rmod = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next), e) ||
        fn_arg_name_check("mod_enum", DOC_MOD_ENUM, 3, query->rval, e))
        goto fail1;

    name = ti_names_from_raw((ti_raw_t *) query->rval);
    if (!name)
    {
        ex_set_mem(e);
        goto fail1;
    }

    ti_val_unsafe_drop(query->rval);
    query->rval = NULL;

    if (ti_raw_eq_strn(rmod, "add", 3))
    {
        enum__add(query, enum_, name, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "def", 3))
    {
        enum__def(query, enum_, name, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "del", 3))
    {
        enum__del(query, enum_, name, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "mod", 3))
    {
        enum__mod(query, enum_, name, nd, e);
        goto done;
    }

    if (ti_raw_eq_strn(rmod, "ren", 3))
    {
        enum__ren(query, enum_, name, nd, e);
        goto done;
    }

    ex_set(e, EX_VALUE_ERROR,
            "function `mod_enum` expects argument 2 to be "
            "`add`, `del`, `mod` or `ren` but got `%.*s` instead"DOC_MOD_ENUM,
            rmod->n, (const char *) rmod->data);

done:
    if (e->nr == 0)
    {
        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_nil_get();
    }
    ti_name_unsafe_drop(name);

fail1:
    ti_val_unsafe_drop((ti_val_t *) rmod);
fail0:
    ti_enum_unlock(enum_, true /* lock is set for sure */);
    return e->nr;
}
