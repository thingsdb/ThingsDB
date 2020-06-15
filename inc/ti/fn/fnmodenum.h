#include <ti/fn/fn.h>

static void enum__add(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

    static const char * fnname = "mod_enum` with task `add";
    cleri_children_t * child;
    ti_task_t * task;
    ti_member_t * member;

    if (fn_nargs(fnname, DOC_MOD_ENUM_ADD, 4, nargs, e))
        return;

    child = nd->children->next->next->next->next->next->next;

    if (ti_do_statement(query, child->node, e))
        return;

    member = ti_member_create(enum_, name, query->rval, e);
    if (!member)
        return;

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        goto fail0;

    /* update modified time-stamp */
    enum_->modified_at = util_now_tsec();

    /* query->rval might be null; when there are no instances */
    if (ti_task_add_mod_enum_add(task, member))
    {
        ex_set_mem(e);
        goto fail0;
    }
    return;  /* success */

fail0:
    ti_member_del(member);  /* failed */
}

static void enum__def(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);

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

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return;

    /* update modified time-stamp */
    enum_->modified_at = util_now_tsec();

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
    const int nargs = langdef_nd_n_function_params(nd);
    static const char * fnname = "mod_enum` with task `del";
    ti_member_t * member = ti_enum_member_by_strn(enum_, name->str, name->n);
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_ENUM_DEL, 3, nargs, e))
        return;

    if (!member)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member `%s`",
                enum_->name, name->str);
        return;
    }

    if (enum_->members->n == 1)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "cannot delete `%s{%s}` as this is the last enum member",
                enum_->name, name->str);
        return;
    }

    if (member->ref > 1)
    {
        ex_set(e, EX_OPERATION_ERROR,
                "enum member `%s{%s}` is still being used",
                enum_->name, name->str);
        return;
    }

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return;

    /* update modified time-stamp */
    enum_->modified_at = util_now_tsec();

    if (ti_task_add_mod_enum_del(task, member))
        ex_set_mem(e);

    ti_member_del(member);
}

static void enum__mod(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    static const char * fnname = "mod_enum` with task `mod";
    cleri_children_t * child;
    ti_member_t * member = ti_enum_member_by_strn(enum_, name->str, name->n);
    ti_task_t * task;

    if (fn_nargs(fnname, DOC_MOD_ENUM_MOD, 4, nargs, e))
        return;

    child = nd->children->next->next->next->next->next->next;

    if (!member)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member `%s`",
                enum_->name, name->str);
        return;
    }

    if (ti_do_statement(query, child->node, e))
        return;

    if (ti_opr_eq(member->val, query->rval))
        return;  /* do nothing, values are equal */

    if (ti_member_set_value(member, query->rval, e))
        return;

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return;

    /* update modified time-stamp */
    enum_->modified_at = util_now_tsec();

    if (ti_task_add_mod_enum_mod(task, member))
        ex_set_mem(e);
}

static void enum__ren(
        ti_query_t * query,
        ti_enum_t * enum_,
        ti_name_t * name,
        cleri_node_t * nd,
        ex_t * e)
{
    const int nargs = langdef_nd_n_function_params(nd);
    static const char * fnname = "mod_enum` with task `ren";
    cleri_children_t * child;
    ti_member_t * member = ti_enum_member_by_strn(enum_, name->str, name->n);
    ti_task_t * task;
    ti_raw_t * rname;

    if (fn_nargs(fnname, DOC_MOD_ENUM_REN, 4, nargs, e))
        return;

    child = nd->children->next->next->next->next->next->next;

    if (!member)
    {
        ex_set(e, EX_LOOKUP_ERROR,
                "enum `%s` has no member `%s`",
                enum_->name, name->str);
        return;
    }

    if (ti_do_statement(query, child->node, e) ||
        fn_arg_str_slow(fnname, DOC_MOD_ENUM_REN, 4, query->rval, e))
        return;

    if (ti_opr_eq((ti_val_t *) member->name, query->rval))
        return;  /* do nothing, name is equal to current name */

    rname = (ti_raw_t *) query->rval;

    if (ti_member_set_name(member, (const char *) rname->data, rname->n, e))
        return;

    task = ti_task_get_task(query->ev, query->collection->root, e);
    if (!task)
        return;

    /* update modified time-stamp */
    enum_->modified_at = util_now_tsec();

    if (ti_task_add_mod_enum_ren(task, member))
        ex_set_mem(e);
}

static int do__f_mod_enum(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    ti_enum_t * enum_;
    ti_name_t * name;
    ti_raw_t * rmod;
    cleri_children_t * child = nd->children;
    const int nargs = langdef_nd_n_function_params(nd);

    if (fn_not_collection_scope("mod_enum", query, e) ||
        fn_nargs_min("mod_enum", DOC_MOD_ENUM, 3, nargs, e) ||
        ti_do_statement(query, child->node, e) ||
        fn_arg_str_slow("mod_enum", DOC_MOD_ENUM, 1, query->rval, e))
        return e->nr;

    enum_ = ti_enums_by_raw(query->collection->enums, (ti_raw_t *) query->rval);
    if (!enum_)
        return ti_raw_err_not_found((ti_raw_t *) query->rval, "enum", e);

    if (ti_enum_try_lock(enum_, e))
        return e->nr;

    ti_val_drop(query->rval);
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_name_check("mod_enum", DOC_MOD_ENUM, 2, query->rval, e))
        goto fail0;

    rmod = (ti_raw_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, (child = child->next->next)->node, e) ||
        fn_arg_name_check("mod_enum", DOC_MOD_ENUM, 3, query->rval, e))
        goto fail1;

    name = ti_names_from_raw((ti_raw_t *) query->rval);
    if (!name)
    {
        ex_set_mem(e);
        goto fail1;
    }

    ti_val_drop(query->rval);
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
            (int) rmod->n, (const char *) rmod->data);

done:
    if (e->nr == 0)
    {
        ti_val_drop(query->rval);
        query->rval = (ti_val_t *) ti_nil_get();
    }
    ti_name_drop(name);

fail1:
    ti_val_drop((ti_val_t *) rmod);
fail0:
    ti_enum_unlock(enum_, true /* lock is set for sure */);
    return e->nr;
}
