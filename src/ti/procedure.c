/*
 * ti/procedure.c
 */
#include <assert.h>
#include <langdef/langdef.h>
#include <ti/procedure.h>
#include <ti/name.h>
#include <ti/names.h>
#include <ti/do.h>
#include <ti/ncache.h>
#include <ti/val.h>
#include <ti/nil.h>
#include <ti/prop.h>
#include <tiinc.h>
#include <stdlib.h>
#include <ctype.h>
#include <util/logger.h>

#define procedure__skip_white_space() \
for (;n && isspace(*pt); ++pt, --n)

static ti_procedure_t * procedure__create(void)
{
    ti_procedure_t * procedure = malloc(sizeof(ti_procedure_t));
    if (!procedure)
        return NULL;

    procedure->ref = 1;
    procedure->flags = 0;
    procedure->def = NULL;
    procedure->name = NULL;
    procedure->arguments = vec_new(0);
    procedure->val_cache = NULL;
    procedure->node = NULL;

    if (!procedure->arguments)
    {
        ti_procedure_drop(procedure);
        return NULL;
    }

    return procedure;
}

void ti_procedure_drop(ti_procedure_t * procedure)
{
    if (!procedure || --procedure->ref)
        return;

    ti_val_drop((ti_val_t *) procedure->def);
    ti_val_drop((ti_val_t *) procedure->name);
    vec_destroy(procedure->arguments, (vec_destroy_cb) ti_prop_destroy);
    vec_destroy(procedure->val_cache, (vec_destroy_cb) ti_val_drop);

    if (procedure->node)
        cleri__node_free(procedure->node);
    free(procedure);
}

ti_procedure_t * ti_procedure_from_raw(
        ti_raw_t * def,
        ti_syntax_t * syntax,
        ex_t * e)
{
    size_t n = def->n, count;
    cleri_parse_t * res = NULL;
    char * start, * pt = (char *) def->data;
    ti_name_t * argname;
    ti_prop_t * prop;
    ;
    ti_procedure_t * procedure = procedure__create();
    if (!procedure)
        goto alloc_error;

    procedure->def = def;
    ti_incref(def);

    procedure__skip_white_space();

    start = pt;

    if (!n || (!isalpha(*pt) && *pt != '_'))
        goto invalid_name;

    ++pt;
    --n;

    for(count = 1; n && (isalnum(*pt) || *pt == '_'); ++count, ++pt, --n);

    procedure->name = ti_raw_from_strn(start, count);
    if (!procedure->name)
        goto alloc_error;

    procedure__skip_white_space();

    if (!n || *pt != '(')
        goto missing_arguments;

    while(1)
    {
        ++pt;
        --n;

        procedure__skip_white_space();

        if (n && *pt == ')')
            break;

        if (!n || *pt != '$')
            goto no_tmp_argument;

        start = pt;

        ++pt;
        --n;

        if (!n || (!isalpha(*pt) && *pt != '_'))
            goto invalid_argument;

        ++pt;
        --n;

        for(count = 2; n && (isalnum(*pt) || *pt == '_'); ++count, ++pt, --n);

        argname = ti_names_get(start, count);
        if (!argname)
            goto alloc_error;

        prop = ti_prop_create(argname, NULL);
        if (!prop)
        {
            ti_name_drop(argname);
            goto alloc_error;
        }

        if (vec_push(&procedure->arguments, prop))
        {
            ti_prop_destroy(prop);
            goto alloc_error;
        }

        procedure__skip_white_space();

        if (n && *pt == ',')
            continue;

        if (n && *pt == ')')
            break;

        goto missing_arguments_end;
    }

    ++pt;
    --n;

    procedure__skip_white_space();

    if (!n || *pt != '{')
        goto missing_body;

    ++pt;
    --n;

    procedure__skip_white_space();
    start = pt;

    pt += n;
    while (1)
    {
        if (pt <= start)
            goto missing_body_end;
        --pt;
        assert(n);  /* must be > 0 */
        --n;
        if (*pt == '}')
            break;
    }

    *pt = '\0';     /* temporary set null */
    res = cleri_parse2(
            ti()->langdef,
            start,
            CLERI_FLAG_EXPECTING_DISABLED|
            CLERI_FLAG_EXCLUDE_OPTIONAL);  /* only error position */
    *pt = '}';      /* restore `}` */

    if (!res)
        goto alloc_error;

    if (!res->is_valid)
    {
        ex_set(e, EX_BAD_DATA,
                "cannot create procedure: error in body at position `%zu`"
                TI_SEE_DOC("#new_procedure"),
                (def->n - n) + res->pos);
        goto failed;
    }

    procedure->node = res->tree         /* root */
            ->children->node            /* sequence <comment, list> */
            ->children->next->node;     /* list statements */
    /*
     * The node will be destroyed when the procedure is destroyed, make sure
     * it stays alive after the parse result is destroyed.
     */
    ++procedure->node->ref;

    ti_syntax_investigate(syntax, procedure->node);

    if (syntax->flags & TI_SYNTAX_FLAG_EVENT)
        procedure->flags |= TI_PROCEDURE_FLAG_EVENT;

    if (syntax->val_cache_n)
    {
        procedure->val_cache = vec_new(syntax->val_cache_n);
        if (!procedure->val_cache || ti_ncache_gen_primitives(
                procedure->val_cache,
                procedure->node,
                e))
            goto alloc_error;
    }

    cleri_parse_free(res);
    return procedure;

invalid_name:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: invalid name at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            def->n - n);
    goto failed;

missing_arguments:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: expecting `(` at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            def->n - n);
    goto failed;

no_tmp_argument:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: "
            "argument at position `%d` should start with a `$`"
            TI_SEE_DOC("#new_procedure"),
            def->n - n);
    goto failed;

invalid_argument:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: invalid argument name at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            def->n - n);
    goto failed;

missing_arguments_end:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: missing `)` at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            def->n - n);
    goto failed;

missing_body:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: expecting `{` at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            def->n - n);
    goto failed;

missing_body_end:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: missing `}` to close the procedure body"
            TI_SEE_DOC("#new_procedure"));
    goto failed;

alloc_error:
    ex_set_alloc(e);
    goto failed;

failed:
    if (res)
        cleri_parse_free(res);
    ti_procedure_drop(procedure);
    return NULL;
}

ti_procedure_t * ti_procedure_from_strn(
        const char * str,
        size_t n,
        ti_syntax_t * syntax,
        ex_t * e)
{
    ti_procedure_t * procedure;
    ti_raw_t * def = ti_raw_from_strn(str, n);
    if (!def)
    {
        ex_set_alloc(e);
        return NULL;
    }

    procedure = ti_procedure_from_raw(def, syntax, e);
    ti_val_drop((ti_val_t *) def);
    return procedure;
}

int ti_procedure_call(ti_procedure_t * procedure, ti_query_t * query, ex_t * e)
{
    size_t argn = procedure->arguments->n;
    cleri_children_t * child = procedure->node->children;
    vec_t * tmpvars;

    assert (query->rval == NULL);

    if (procedure->flags & TI_RPOCEDURE_FLAG_LOCK)
    {
        ex_set(e, EX_BAD_DATA,
                "procedures cannot be called recursively"
                TI_SEE_DOC("#call"));
        return e->nr;
    }

    if (!child)
    {
        query->rval = (ti_val_t *) ti_nil_get();
        return e->nr;
    }

    /* lock procedure */
    procedure->flags |= TI_RPOCEDURE_FLAG_LOCK;

    /* store and replace temporary variable */
    tmpvars = query->tmpvars;
    query->tmpvars = procedure->arguments;

    while (1)
    {
        assert (child->node->cl_obj->gid == CLERI_GID_SCOPE);

        if (ti_do_scope(query, child->node, e))
            break;

        if (!child->next || !(child = child->next->next))
            break;

        ti_val_drop(query->rval);
        query->rval = NULL;
    }

    /* unlock procedure */
    procedure->flags &= ~TI_RPOCEDURE_FLAG_LOCK;

    /* restore original temporary variable */
    while (query->tmpvars->n > argn)
        ti_prop_destroy(vec_pop(query->tmpvars));
    procedure->arguments = query->tmpvars;   /* may be re-allocated */
    query->tmpvars = tmpvars;

    return e->nr;
}

int ti_procedure_run(ti_query_t * query, ex_t * e)
{
    ti_procedure_t * procedure = query->procedure;
    size_t idx = 0;

    assert (procedure);
    assert (procedure->arguments->n == query->val_cache->n);

    for (vec_each(procedure->arguments, ti_prop_t, prop), ++idx)
        prop->val = query->val_cache->data[idx];

    return ti_procedure_call(procedure, query, e);
}
