/*
 * ti/closure.h
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <ti/closure.h>
#include <ti/closure.inline.h>
#include <ti/query.inline.h>
#include <ti/do.h>
#include <ti/names.h>
#include <ti/ncache.h>
#include <ti/nil.h>
#include <ti/raw.inline.h>
#include <ti/regex.h>
#include <ti/val.inline.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/logger.h>
#include <util/strx.h>
#include <cleri/node.inline.h>

#define CLOSURE__QBOUND (TI_VFLAG_CLOSURE_BTSCOPE|TI_VFLAG_CLOSURE_BCSCOPE)

static inline _Bool closure__is_unbound(ti_closure_t * closure)
{
    return (~closure->flags & CLOSURE__QBOUND) == CLOSURE__QBOUND;
}

static cleri_node_t * closure__node_from_strn(
        ti_qbind_t * syntax,
        const char * str,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    ti_ncache_t * ncache;
    cleri_parse_t * res;
    cleri_node_t * node, * statement;
    char * query = strndup(str, n);
    if (!query)
    {
        ex_set_mem(e);
        return NULL;
    }

    res = cleri_parse2(ti()->langdef, query, TI_CLERI_PARSE_FLAGS);
    if (!res)
    {
        ex_set_mem(e);
        goto fail0;
    }

    if (!res->is_valid)
    {
        ex_set(e, EX_SYNTAX_ERROR, "invalid syntax in closure");
        goto fail1;
    }

    node = res->tree->children->node        /* Sequence (START) */
            ->children->next->node;         /* List of statements */

    /* we should have exactly one statement */
    if (!node->children || node->children->next)
    {
        ex_set(e, EX_BAD_DATA, "closure is expecting exactly one node");
        goto fail1;
    }

    node = node                             /* List of statements */
            ->children->node                /* Sequence - statement */
            ->children->node                /* expression */
            ->children->next->node          /* Choice - immutable */
            ->children->node;               /* closure */

    if (node->cl_obj->gid != CLERI_GID_T_CLOSURE)
    {
        ex_set(e, EX_LOOKUP_ERROR, "node is not a closure");
        goto fail1;
    }

    /*  closure = Sequence('|', List(name, opt=True), '|', statement)  */
    statement = node->children->next->next->next->node;
    ti_qbind_probe(syntax, statement);

    ncache = ti_ncache_create(query, syntax->val_cache_n);
    if (!ncache)
    {
        ex_set_mem(e);
        goto fail1;
    }
    query = NULL;  /* ownership is moved to ncache */
    node->data = ncache;
    if (ti_ncache_gen_node_data(syntax, ncache->val_cache, statement, e))
        goto fail2;

    /* make sure the node gets an extra reference so it will be kept */
    ++node->ref;

    cleri_parse_free(res);
    return node;

fail2:
    ti_ncache_destroy(ncache);
fail1:
    cleri_parse_free(res);
fail0:
    free(query);
    return NULL;
}

static void closure__node_to_buf(cleri_node_t * nd, char * buf, size_t * n)
{
    switch (nd->cl_obj->tp)
    {
    case CLERI_TP_KEYWORD:
    case CLERI_TP_TOKEN:
    case CLERI_TP_TOKENS:
    case CLERI_TP_REGEX:
        memcpy(buf + (*n), nd->str, nd->len);
        (*n) += nd->len;
        return;
    case CLERI_TP_SEQUENCE:
    case CLERI_TP_OPTIONAL:
    case CLERI_TP_CHOICE:
    case CLERI_TP_LIST:
    case CLERI_TP_REPEAT:
    case CLERI_TP_PRIO:
    case CLERI_TP_RULE:
    case CLERI_TP_THIS:
    case CLERI_TP_REF:
    case CLERI_TP_END_OF_STATEMENT:
        break;
    }

    for (cleri_children_t * child = nd->children; child; child = child->next)
        closure__node_to_buf(child->node, buf, n);
}

static vec_t * closure__create_vars(ti_closure_t * closure)
{
    vec_t * vars;
    size_t n;
    cleri_children_t * child, * first;

    first = closure->node               /* sequence */
            ->children->next->node      /* list */
            ->children;                 /* first child */

    for(n = 0, child = first;
        child && ++n;
        child = child->next? child->next->next : NULL);

    vars = vec_new(n);
    if (!vars)
        return NULL;

    for (child = first; child; child = child->next->next)
    {
        ti_val_t * val = (ti_val_t *) ti_nil_get();
        ti_name_t * name = ti_names_get(child->node->str, child->node->len);
        ti_prop_t * prop;
        if (!name)
            goto failed;

        prop = ti_prop_create(name, val);
        if (!prop)
        {
            ti_name_drop(name);
            ti_val_drop(val);
            goto failed;
        }

        VEC_push(vars, prop);
        if (!child->next)
            break;
    }

    return vars;

failed:
    vec_destroy(vars, (vec_destroy_cb) ti_prop_destroy);
    return NULL;
}

/*
 * Return a closure which is bound to the query. The node for this closure can
 * only be used for as long as the 'query' exists in memory. If the closure
 * will be stored for later usage, a call to `ti_closure_unbound` must be
 * made.
 */
ti_closure_t * ti_closure_from_node(cleri_node_t * node, uint8_t flags)
{
    ti_closure_t * closure = malloc(sizeof(ti_closure_t));
    if (!closure)
        return NULL;

    closure->ref = 1;
    closure->tp = TI_VAL_CLOSURE;
    closure->flags = flags;
    closure->depth = 0;
    closure->node = node;
    closure->stacked = NULL;
    closure->vars = closure__create_vars(closure);
    if (!closure->vars)
    {
        ti_closure_destroy(closure);
        return NULL;
    }
    return closure;
}

ti_closure_t * ti_closure_from_strn(
        ti_qbind_t * syntax,
        const char * str,
        size_t n, ex_t * e)
{
    ti_closure_t * closure = malloc(sizeof(ti_closure_t));
    if (!closure)
        return NULL;

    closure->ref = 1;
    closure->tp = TI_VAL_CLOSURE;
    closure->depth = 0;
    closure->node = closure__node_from_strn(syntax, str, n, e);
    closure->flags = syntax->flags & TI_QBIND_FLAG_EVENT
            ? TI_VFLAG_CLOSURE_WSE
            : 0;
    closure->stacked = NULL;
    closure->vars = closure->node ? closure__create_vars(closure) : NULL;

    if (!closure->node || !closure->vars)
    {
        ti_closure_destroy(closure);
        return NULL;
    }

    return closure;
}

void ti_closure_destroy(ti_closure_t * closure)
{
    if (!closure)
        return;

    if (closure__is_unbound(closure) && closure->node)
    {
        ti_ncache_destroy((ti_ncache_t *) closure->node->data);
        cleri__node_free(closure->node);
    }

    vec_destroy(closure->vars, (vec_destroy_cb) ti_prop_destroy);
    free(closure->stacked);
    free(closure);
}

int ti_closure_unbound(ti_closure_t * closure, ex_t * e)
{
    cleri_node_t * node;

    if (closure__is_unbound(closure))
        return 0;

    ti_qbind_t syntax = {
            .val_cache_n = 0,
            .flags = closure->flags & TI_VFLAG_CLOSURE_BTSCOPE
                ? TI_QBIND_FLAG_THINGSDB
                : TI_QBIND_FLAG_COLLECTION,
    };

    node = closure__node_from_strn(
            &syntax,
            closure->node->str,
            closure->node->len, e);
    if (!node)
        return e->nr;

    closure->flags = syntax.flags & TI_QBIND_FLAG_EVENT
            ? TI_VFLAG_CLOSURE_WSE
            : 0;
    closure->node = node;

    return e->nr;
}

int ti_closure_to_pk(ti_closure_t * closure, msgpack_packer * pk)
{
    char * buf;
    size_t n = 0;
    int rc;
    if (!closure__is_unbound(closure))
    {
        return -(
            msgpack_pack_map(pk, 1) ||
            mp_pack_strn(pk, TI_KIND_S_CLOSURE, 1) ||
            mp_pack_strn(pk, closure->node->str, closure->node->len)
        );
    }

    buf = ti_closure_char(closure, &n);
    if (!buf)
        return -1;

    rc = -(
        msgpack_pack_map(pk, 1) ||
        mp_pack_strn(pk, TI_KIND_S_CLOSURE, 1) ||
        mp_pack_strn(pk, buf, n)
    );

    free(buf);
    return rc;
}

char * ti_closure_char(ti_closure_t * closure, size_t * n)
{
    char * buf;
    buf = malloc(closure->node->len);
    if (!buf)
        return NULL;

    closure__node_to_buf(closure->node, buf, n);
    return buf;
}

int ti_closure_inc(ti_closure_t * closure, ti_query_t * query, ex_t * e)
{
    uint32_t n = closure->vars->n;

    switch (closure->depth)
    {
    case TI_CLOSURE_MAX_RECURSION_DEPTH:
        ex_set(e, EX_OPERATION_ERROR,
                "maximum recursion depth exceeded"DOC_CLOSURE);
        return -1;
    case 0:
        if (n && vec_extend(&query->vars, closure->vars->data, n))
            goto err_alloc;
        break;
    default:
        if (n)
        {
            uint32_t pos = closure->stack_pos[closure->depth-1] - n;
            uint32_t to = query->vars->n - n;

            if (vec_reserve(&closure->stacked, n))
                goto err_alloc;

            for (vec_each(closure->vars, ti_prop_t, p))
            {
                VEC_push(closure->stacked, p->val);
                ti_incref(p->val);
            }
            vec_move(query->vars, pos, n, to);
        }
    }

    closure->stack_pos[closure->depth++] = query->vars->n;
    return 0;

err_alloc:
    ex_set_mem(e);
    return -1;
}

/* Unlock use, resets all variable to `nil` */
void ti_closure_dec(ti_closure_t * closure, ti_query_t * query)
{
    uint32_t n = closure->vars->n;
    uint32_t pos = closure->stack_pos[--closure->depth];

    /* drop temporary added props */
    while (query->vars->n > pos)
        ti_query_var_drop_gc(VEC_pop(query->vars), query);

    if (closure->depth)
    {
        uint32_t to = closure->stack_pos[closure->depth-1];

        /* restore property values */
        for (vec_each_rev(closure->vars, ti_prop_t, p))
        {
            ti_query_val_gc(p->val, query);
            ti_val_drop(p->val);
            p->val = VEC_pop(closure->stacked);
        }

        /* move the property values to the correct place on the stack */
        vec_move(query->vars, pos-n, n, to-n);
    }
    else
    {
        /* restore `old` size so closure keeps ownership of it's own props */
        query->vars->n -= n;

        /* reset props */
        for (vec_each(closure->vars, ti_prop_t, p))
        {
            ti_query_val_gc(p->val, query);
            ti_val_drop(p->val);
            p->val = (ti_val_t *) ti_nil_get();
        }
    }
}

int ti_closure_vars_nameval(
        ti_closure_t * closure,
        ti_name_t * name,
        ti_val_t * val,
        ex_t * e)
{
    ti_prop_t * prop;
    switch(closure->vars->n)
    {
    default:
    case 2:
        prop = vec_get(closure->vars, 1);
        ti_incref(val);
        ti_val_drop(prop->val);
        prop->val = val;
        /*
         * Re-assign variable since we require a copy of lists and sets.
         */
        if (ti_val_make_variable(&prop->val, e))
            return e->nr;
        /* fall through */
    case 1:
        prop = vec_get(closure->vars, 0);
        ti_incref(name);
        ti_val_drop(prop->val);
        prop->val = (ti_val_t *) name;
        /* fall through */
    case 0:
        break;
    }
    return 0;
}

int ti_closure_vars_val_idx(ti_closure_t * closure, ti_val_t * v, int64_t i)
{
    ti_prop_t * prop;
    switch(closure->vars->n)
    {
    default:
    case 2:
        prop = vec_get(closure->vars, 1);
        ti_val_drop(prop->val);
        prop->val = (ti_val_t *) ti_vint_create(i);
        if (!prop->val)
            return -1;
        /* fall through */
    case 1:
        prop = vec_get(closure->vars, 0);
        ti_incref(v);
        ti_val_drop(prop->val);
        prop->val = v;
        /* fall through */
    case 0:
        break;
    }
    return 0;
}

int ti_closure_call(
        ti_closure_t * closure,
        ti_query_t * query,
        vec_t * args,
        ex_t * e)
{
    assert (closure);
    assert (closure->vars);

    size_t i = 0;

    if (args->n != closure->vars->n)
    {
        ex_set(e, EX_NUM_ARGUMENTS,
                "this closure takes %d %s but %d %s given",
                closure->vars->n,
                closure->vars->n == 1 ? "argument" : "arguments",
                args->n,
                args->n == 1 ? "was" : "were");
        return e->nr;
    }

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_inc(closure, query, e))
        return e->nr;

    for (vec_each(closure->vars, ti_prop_t, prop), ++i)
    {
        ti_val_drop(prop->val);
        prop->val = args->data[i];
        ti_incref(prop->val);
    }

    (void) ti_closure_do_statement(closure, query, e);
    ti_closure_dec(closure, query);

    return e->nr;
}

ti_raw_t * ti_closure_doc(ti_closure_t * closure)
{
    ti_raw_t * doc = NULL;
    cleri_node_t * node = ti_closure_statement(closure)
            ->children->node                /* expression */
            ->children->next->node;         /* the choice */

    while ((node->cl_obj->gid == CLERI_GID_VAR_OPT_MORE ||
            node->cl_obj->gid == CLERI_GID_NAME_OPT_MORE
        ) &&
            node->children->next &&
            node->children->next->node->cl_obj->gid == CLERI_GID_FUNCTION)
    {
        /*
         * If the scope is a function, get the first argument, for example:
         *   || wse({
         *      "Read this doc string...";
         *   });
         */
        node = node->children->next->node       /* function */
                ->children->next->node;         /* arguments */

        if (node->children)
            node = node->children->node         /* statement */
            ->children->node                    /* expression */
            ->children->next->node;             /* the choice */
        assert (node);
    }

    if (node->cl_obj->gid != CLERI_GID_BLOCK)
        goto done;

    node = node->children->next->next   /* node=block */
            ->node->children            /* node=list mi=1 */
            ->node->children            /* node=statement */
            ->node->children->next      /* node=expression */
            ->node;                     /* node=the choice */

    if (node->cl_obj->gid != CLERI_GID_IMMUTABLE ||
        node->children->node->cl_obj->gid != CLERI_GID_T_STRING)
        goto done;

    doc = node->children->node->data;
    if (doc)
        /* from cache */
        ti_incref(doc);
    else
        /* create and return a new string */
        doc = ti_str_from_ti_string(node->str, node->len);

done:
    return doc ? doc : (ti_raw_t *) ti_val_empty_str();
}

#define CLOSURE__SN_FMT(__buf, __sz, __n, __fmt, ...)                       \
do {                                                                        \
    int __nchars = snprintf(__buf, __n, __fmt, ##__VA_ARGS__);              \
    if (__nchars < 0) return -1;                                            \
    __sz += __nchars;                                                       \
    if ((size_t) __nchars < __n) { __buf += __nchars; __n -= __nchars; }    \
} while (0)

#define CLOSURE__NODE_FMT(__buf, __sz, __n, __node)             \
do {                                                            \
    size_t __nchars = __node->len;                              \
    const char * __str = __node->str;                           \
    __sz += __nchars;                                           \
    if (__nchars < __n)                                         \
    {                                                           \
        (void) memcpy(__buf, __str, __nchars);                  \
        __buf += __nchars;                                      \
        __n -= __nchars;                                        \
    }                                                           \
} while (0)

//static int closure__scope_fmt(
//        cleri_node_t * node,
//        char ** buf,
//        size_t * n,
//        size_t * indent)
//{
//
//}
//
//static int closure__closure_fmt(
//        cleri_node_t * node,
//        char ** buf,
//        size_t * n,
//        size_t * indent)
//{
//    int sz = 0;
//    cleri_children_t * child = node->children->next->node->children;
//
//    CLOSURE__SN_FMT(*buf, sz, *n, "|");
//
//    while (child)
//    {
//        CLOSURE__NODE_FMT(*buf, sz, *n, child->node);
//
//        if (!child->next || !(child = child->next->next))
//            break;
//
//        CLOSURE__SN_FMT(*buf, sz, *n, ", ");
//    }
//
//    CLOSURE__SN_FMT(*buf, sz, *n, "| ");
//
//
//    return sz;
//}
