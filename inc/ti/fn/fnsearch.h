#include <ti/fn/fn.h>

typedef struct
{
    uint8_t deep;
    uint8_t limit;
    ex_t * e;
    ti_thing_t * needle;
    ti_thing_t * parent;
    vec_t * results;
} search__walk_t;

typedef struct
{
    ti_raw_t * key;
    ti_val_t * root;
    search__walk_t * search;
} search__walk_set_t;

static int do__search_walk(
        ti_raw_t * key,
        ti_val_t * val,
        search__walk_t * w);

static int do__search_option(
        ti_raw_t * key,
        ti_val_t * val,
        search__walk_t * w)
{
    if (ti_raw_eq_strn(key, "deep", 4))
        return ti_deep_from_val(val, &w->deep, w->e);

    if (ti_raw_eq_strn(key, "limit", 5))
    {
        int64_t limiti;

        if (!ti_val_is_int(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                    "expecting `limit` to be of type `"TI_VAL_INT_S"` but "
                    "got type `%s` instead"DOC_THING_SEARCH,
                    ti_val_str(val));
            return w->e->nr;
        }

        limiti = VINT(val);

        if (limiti < 0 || limiti > TI_MAX_SEARCH_LIMIT)
        {
            ex_set(w->e, EX_VALUE_ERROR,
                    "expecting a `limit` value between 0 and %d "
                    "but got %"PRId64" instead",
                    TI_MAX_SEARCH_LIMIT, limiti);
            return w->e->nr;
        }

        w->limit = (uint8_t) limiti;
        return 0;
    }

    ex_set(w->e, EX_VALUE_ERROR,
            "invalid search option `%.*s`"DOC_THING_SEARCH, key->n, key->data);

    return w->e->nr;
}

static int search__add_result(
        ti_thing_t * parent,
        ti_raw_t * key,
        ti_val_t * root,
        search__walk_t * w)
{
    ti_val_t * key_type, * parent_type;
    ti_name_t * parent_name = \
            (ti_name_t *) ti_val_borrow_parent_name();
    ti_name_t * parent_type_name = \
            (ti_name_t *) ti_val_borrow_parent_type_name();
    ti_name_t * key_name = \
            (ti_name_t *) ti_val_borrow_key_name();
    ti_name_t * key_type_name = \
            (ti_name_t *) ti_val_borrow_key_type_name();
    ti_thing_t * thing = \
            ti_thing_o_create(0, 4, w->parent->collection);
    if (!thing)
        return -1;

    parent_type = ti_val_strv((ti_val_t *) parent);
    key_type = ti_val_strv(root);

    if (ti_thing_p_prop_add(thing, parent_name, (ti_val_t *) parent))
    {
        ti_incref(parent_name);
        ti_incref(parent);
    }
    if (ti_thing_p_prop_add(thing, parent_type_name, parent_type))
        ti_incref(parent_type_name);
    else
        ti_val_drop(parent_type);

    if (ti_thing_p_prop_add(thing, key_name, (ti_val_t *) key))
    {
        ti_incref(key_name);
        ti_incref(key);
    }
    if (ti_thing_p_prop_add(thing, key_type_name, key_type))
        ti_incref(key_type_name);
    else
        ti_val_drop(key_type);

    if (ti_thing_n(thing) != 4)
    {
        ti_val_drop((ti_val_t *) thing);
        ex_set_mem(w->e);
        return w->e->nr;
    }

    VEC_push(w->results, thing);
    return --w->limit == 0;
}

static int search__do_thing(
        ti_thing_t * thing,
        ti_val_t * root,
        ti_raw_t * key,
        search__walk_t * w)
{
    ti_thing_t * prev;
    int rc;

    if (thing->flags & TI_VFLAG_LOCK)
        return 0;

    if (thing == w->needle)
        if (search__add_result(w->parent, key, root, w))
            return 1;

    if (w->deep == 1)
        return 0;

    --w->deep;
    thing->flags |= TI_VFLAG_LOCK;
    prev = w->parent;
    w->parent = thing;
    rc = ti_thing_walk(
         (ti_thing_t *) thing,
         (ti_thing_item_cb) do__search_walk,
         w);
    w->parent = prev;
    ++w->deep;
    thing->flags &= ~TI_VFLAG_LOCK;
    return rc;
}

static inline int search__walk_set(ti_thing_t * thing, search__walk_set_t * w)
{
    return search__do_thing(thing, w->root, w->key, w->search);
}

static int do__search_thing(
        ti_raw_t * key,
        ti_val_t * root,
        ti_val_t * val,
        search__walk_t * w)
{
    switch ((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
    case TI_VAL_INT:
    case TI_VAL_FLOAT:
    case TI_VAL_BOOL:
    case TI_VAL_DATETIME:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_ROOM:
    case TI_VAL_TASK:
    case TI_VAL_ERROR:
    case TI_VAL_MPDATA:
    case TI_VAL_CLOSURE:
        return 0;
    case TI_VAL_WRAP:
        return search__do_thing(((ti_wrap_t *) val)->thing, root, key, w);
    case TI_VAL_THING:
        return search__do_thing((ti_thing_t *) val, root, key, w);
    case TI_VAL_MEMBER:
        return ti_val_is_thing(VMEMBER(val))
            ? search__do_thing((ti_thing_t *) VMEMBER(val), root, key, w)
            : 0;
    case TI_VAL_ARR:
        if (ti_varr_may_have_things((ti_varr_t *) val))
            for (vec_each(VARR(val), ti_val_t, v))
                if (do__search_thing(key, val, v, w))
                    return 1;
        return 0;
    case TI_VAL_SET:
    {
        search__walk_set_t ws = {
                .key = key,
                .root = root,
                .search = w,
        };
        return imap_walk(VSET(val), (imap_cb) search__walk_set, &ws);
    }
    case TI_VAL_TEMPLATE:
    case TI_VAL_FUTURE:
    case TI_VAL_MODULE:
        return 0;
    }
    assert(0);
    return 0;
}

static int do__search_walk(
        ti_raw_t * key,
        ti_val_t * val,
        search__walk_t * w)
{
    return do__search_thing(key, val, val, w);
}

static int do__f_search(ti_query_t * query, cleri_node_t * nd, ex_t * e)
{
    const int nargs = fn_get_nargs(nd);
    search__walk_t search = {
            .limit = 1,
            .deep = 1,
            .e = e,
    };

    if (!ti_val_is_thing(query->rval))
        return fn_call_try("search", query, nd, e);

    if (fn_nargs_range("search", DOC_THING_SEARCH, 1, 2, nargs, e))
        return e->nr;

    search.parent = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (ti_do_statement(query, nd->children, e) ||
        fn_arg_thing("search", DOC_THING_SEARCH, 1, query->rval, e))
        goto fail0;

    search.needle = (ti_thing_t *) query->rval;
    query->rval = NULL;

    if (nargs == 2)
    {
        ti_thing_t * to;

        if (ti_do_statement(query, nd->children->next->next, e) ||
            fn_arg_thing("search", DOC_THING_SEARCH, 2, query->rval, e))
            goto fail1;

        to = (ti_thing_t *) query->rval;

        if (ti_thing_walk(to, (ti_thing_item_cb) do__search_option, &search))
            goto fail1;

        ti_val_unsafe_drop(query->rval);
        query->rval = NULL;
    }

    search.results = vec_new(search.limit);
    if (!search.results)
    {
        ex_set_mem(e);
        goto fail1;
    }

    if (search.deep && search.limit && ti_thing_walk(
            search.parent,
            (ti_thing_item_cb) do__search_walk,
            &search) < 0)
        goto fail1;

    query->rval = (ti_val_t *) ti_varr_from_vec(search.results);
    if (!query->rval)
        ex_set_mem(e);
    else
        search.results = NULL;

    vec_destroy(search.results, (vec_destroy_cb) ti_val_drop);
fail1:
    ti_val_unsafe_drop((ti_val_t *) search.needle);
fail0:
    ti_val_unsafe_drop((ti_val_t *) search.parent);
    return e->nr;
}
