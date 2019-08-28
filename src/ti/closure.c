/*
 * ti/closure.h
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <ti/closure.h>
#include <ti/do.h>
#include <ti/names.h>
#include <ti/ncache.h>
#include <ti/nil.h>
#include <ti/regex.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/logger.h>
#include <util/strx.h>

#define TI_CLOSURE_QBOUND (TI_VFLAG_CLOSURE_BTSCOPE|TI_VFLAG_CLOSURE_BCSCOPE)

static inline _Bool closure__is_unbound(ti_closure_t * closure)
{
    return (~closure->flags & TI_CLOSURE_QBOUND) == TI_CLOSURE_QBOUND;
}

static cleri_node_t * closure__node_from_strn(
        ti_syntax_t * syntax,
        const char * str,
        size_t n,
        ex_t * e)
{
    assert (e->nr == 0);
    ti_ncache_t * ncache;
    cleri_parse_t * res;
    cleri_node_t * node, * scope;
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
            ->children->node                /* Sequence - scope */
            ->children->next->node          /* Choice - immutable */
            ->children->node;               /* closure */

    if (node->cl_obj->gid != CLERI_GID_T_CLOSURE)
    {
        ex_set(e, EX_INDEX_ERROR, "node is not a closure");
        goto fail1;
    }

    /*  closure = Sequence('|', List(name, opt=True), '|', scope)  */
    scope = node->children->next->next->next->node;
    ti_syntax_investigate(syntax, scope);

    ncache = ti_ncache_create(query, syntax->val_cache_n);
    if (!ncache)
    {
        ex_set_mem(e);
        goto fail1;
    }

    node->data = ncache;
    if (ti_ncache_gen_node_data(syntax, ncache->val_cache, scope, e))
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

static void closure__node_to_buf(cleri_node_t * nd, uchar * buf, size_t * n)
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

    for (n = 0, child = first; child && ++n; child = child->next->next)
        if (!child->next)
            break;

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
    closure->node = node;
    closure->vars = closure__create_vars(closure);
    if (!closure->vars)
    {
        ti_closure_destroy(closure);
        return NULL;
    }
    return closure;
}

ti_closure_t * ti_closure_from_strn(
        ti_syntax_t * syntax,
        const char * str,
        size_t n, ex_t * e)
{
    ti_closure_t * closure = malloc(sizeof(ti_closure_t));
    if (!closure)
        return NULL;

    closure->ref = 1;
    closure->tp = TI_VAL_CLOSURE;
    closure->node = closure__node_from_strn(syntax, str, n, e);
    closure->flags = syntax->flags & TI_SYNTAX_FLAG_EVENT
            ? TI_VFLAG_CLOSURE_WSE
            : 0;
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
    free(closure);
}

int ti_closure_unbound(ti_closure_t * closure, ex_t * e)
{
    cleri_node_t * node;
    ti_syntax_t syntax;

    if (closure__is_unbound(closure))
        return 0;

    if (ti_closure_try_lock(closure, e))
        return e->nr;

    ti_syntax_init(&syntax, closure->flags & TI_VFLAG_CLOSURE_BTSCOPE
            ? TI_SYNTAX_FLAG_THINGSDB
            : TI_SYNTAX_FLAG_COLLECTION);

    node = closure__node_from_strn(
            &syntax,
            closure->node->str,
            closure->node->len, e);
    if (!node)
    {
        ti_closure_unlock(closure);
        return e->nr;
    }

    /* overwrite the existing flags, this will also unlock */
    closure->flags = syntax.flags & TI_SYNTAX_FLAG_EVENT
            ? TI_VFLAG_CLOSURE_WSE
            : 0;
    closure->node = node;

    return e->nr;
}

int ti_closure_to_packer(ti_closure_t * closure, qp_packer_t ** packer)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (!closure__is_unbound(closure))
    {
        return -(
            qp_add_map(packer) ||
            qp_add_raw(*packer, (const uchar * ) TI_KIND_S_CLOSURE, 1) ||
            qp_add_raw(
                    *packer,
                    (const uchar * ) closure->node->str,
                    closure->node->len) ||
            qp_close_map(*packer)
        );
    }

    buf = ti_closure_uchar(closure, &n);
    if (!buf)
        return -1;

    rc = -(
        qp_add_map(packer) ||
        qp_add_raw(*packer, (const uchar * ) TI_KIND_S_CLOSURE, 1) ||
        qp_add_raw(*packer, buf, n) ||
        qp_close_map(*packer)
    );

    free(buf);
    return rc;
}

int ti_closure_to_file(ti_closure_t * closure, FILE * f)
{
    uchar * buf;
    size_t n = 0;
    int rc;
    if (!closure__is_unbound(closure))
    {
        return -(
            qp_fadd_type(f, QP_MAP1) ||
            qp_fadd_raw(f, (const uchar * ) TI_KIND_S_CLOSURE, 1) ||
            qp_fadd_raw(f, (const uchar * ) closure->node->str, closure->node->len)
        );
    }
    buf = ti_closure_uchar(closure, &n);
    if (!buf)
        return -1;
    rc = -(
        qp_fadd_type(f, QP_MAP1) ||
        qp_fadd_raw(f, (const uchar * ) TI_KIND_S_CLOSURE, 1) ||
        qp_fadd_raw(f, buf, n)
    );
    free(buf);
    return rc;
}

uchar * ti_closure_uchar(ti_closure_t * closure, size_t * n)
{
    uchar * buf;
    buf = malloc(closure->node->len);
    if (!buf)
        return NULL;

    closure__node_to_buf(closure->node, buf, n);
    return buf;
}

int ti_closure_lock_and_use(
        ti_closure_t * closure,
        ti_query_t * query,
        ex_t * e)
{
    if (vec_extend(&query->vars, closure->vars->data, closure->vars->n))
    {
        ex_set_mem(e);
        return -1;
    }

    closure->locked_n = query->vars->n;
    closure->flags |= TI_VFLAG_LOCK;

    return 0;
}

int ti_closure_vars_prop(ti_closure_t * closure, ti_prop_t * prop, ex_t * e)
{
    size_t n = 0;
    for (vec_each(closure->vars, ti_prop_t, p), ++n)
    {
        switch (n)
        {
        case 0:
            ti_val_drop(p->val);
            p->val = (ti_val_t *) prop->name;
            ti_incref(p->val);
            break;
        case 1:
            ti_val_drop(p->val);
            p->val = prop->val;
            ti_incref(p->val);
            /*
             * Re-assign variable since we require a copy of lists and sets.
             * (otherwise no event will be created)
             *
             * TODO: consider using pointers when assigning to variable, but
             * that requires sets and lists to have a reference to the thing
             * and name they are assigned to.
             */
            if (ti_val_make_assignable(&p->val, e))
                return e->nr;
            break;
        default:
            return 0;
        }
    }
    return 0;
}

int ti_closure_vars_val_idx(ti_closure_t * closure, ti_val_t * v, int64_t i)
{
    size_t n = 0;
    for (vec_each(closure->vars, ti_prop_t, p), ++n)
    {
       switch (n)
       {
       case 0:
           ti_val_drop(p->val);
           p->val = v;
           ti_incref(p->val);
           break;
       case 1:
           ti_val_drop(p->val);
           p->val = (ti_val_t *) ti_vint_create(i);
           if (!p->val)
               return -1;
           break;
       default:
           return 0;
       }
    }
    return 0;
}

/* Unlock use, resets all variable to `nil` */
void ti_closure_unlock_use(ti_closure_t * closure, ti_query_t * query)
{
    assert (query->vars->n >= closure->vars->n);

    closure->flags &= ~TI_VFLAG_LOCK;

    /* drop temporary added props */
    while (query->vars->n > closure->locked_n)
        ti_prop_destroy(vec_pop(query->vars));

    /* restore `old` size so closure keeps ownership of it's own props */
    query->vars->n -= closure->vars->n;

    /* reset props to `nil` */
    for (vec_each(closure->vars, ti_prop_t, p))
    {
        if (!ti_val_is_nil(p->val))
        {
            ti_val_drop(p->val);
            p->val = (ti_val_t *) ti_nil_get();
        }
    }
}

/* cannot be static in-line due to syntax */
int ti_closure_try_wse(ti_closure_t * closure, ti_query_t * query, ex_t * e)
{
    /*
     * The current implementation never set's the `WSE` flag when bound to a
     * scope, but nevertheless we check for explicit only the WSE flag for
     * we might change the code so the WSE flag will be set, even when bound
     */
    if (    ((closure->flags & (
                TI_VFLAG_CLOSURE_BTSCOPE|
                TI_VFLAG_CLOSURE_BCSCOPE|
                TI_VFLAG_CLOSURE_WSE
            )) == TI_VFLAG_CLOSURE_WSE) &&
            (~query->syntax.flags & TI_SYNTAX_FLAG_WSE))
    {
        ex_set(e, EX_BAD_DATA,
                "stored closures with side effects must be "
                "wrapped using `wse(...)`"TI_SEE_DOC("#wse"));
        return -1;
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
        ex_set(e, EX_BAD_DATA,
                "this closure takes %d %s but %d %s given",
                closure->vars->n,
                closure->vars->n == 1 ? "argument" : "arguments",
                args->n,
                args->n == 1 ? "was" : "were");
        return e->nr;
    }

    if (ti_closure_try_wse(closure, query, e) ||
        ti_closure_try_lock_and_use(closure, query, e))
        return e->nr;

    for (vec_each(closure->vars, ti_prop_t, prop), ++i)
    {
        ti_val_drop(prop->val);
        prop->val = args->data[i];
        ti_incref(prop->val);
    }

    (void) ti_closure_do_scope(closure, query, e);
    ti_closure_unlock_use(closure, query);

    return e->nr;
}

ti_raw_t * ti_closure_doc(ti_closure_t * closure)
{
    ti_raw_t * doc = NULL;
    cleri_node_t * node = ti_closure_scope(closure)->children->next
            ->node;                 /* the choice */

    if (node->cl_obj->gid == CLERI_GID_FUNCTION)
    {
        /*
         * If the scope is a function, get the first argument, for example:
         *   || wse({
         *      "Read this doc string...";
         *   });
         */
        node = node->children->next->next->node;  /* arguments */
        if (node->children)
            node = node->children
                ->node->children->next  /* node=first argument (scope) */
                ->node;                 /* the choice */
        assert (node);
    }

    if (node->cl_obj->gid != CLERI_GID_BLOCK)
        goto done;

    if (node->children->next->node->children)
    {
        /* return comment as doc */
        node = node->children->next->node->children->node;
        doc = ti_raw_from_strn(node->str, node->len);
        goto done;
    }

    node = node->children->next->next   /* node=block */
            ->node->children            /* node=list mi=1 */
            ->node->children->next      /* node=scope */
            ->node;                     /* node=the choice */

    if (    node->cl_obj->gid != CLERI_GID_IMMUTABLE ||
            node->children->node->cl_obj->gid != CLERI_GID_T_STRING)
        goto done;

    doc = node->children->node->data;
    ti_incref(doc);

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
