/*
 * ti/procedure.c
 */
#include <ti/procedure.h>
#include <ti/name.h>
#include <ti/val.h>
#include <tiinc.h>
#include <stdlib.h>

#define procedure__skip_white_space() \
for (;n && isspace(*pt); ++pt, --n)

static ti_procedure_t * procedure__create(void)
{
    ti_procedure_t * procedure = malloc(sizeof(ti_procedure_t));
    if (!procedure)
        return NULL;

    procedure->name = NULL;
    procedure->arguments = vec_new(0);
    procedure->nd_val_cache = NULL;
    procedure->node = NULL;
    procedure->body = NULL;

    if (!procedure->arguments)
    {
        ti_procedure_destroy(procedure);
        return NULL;
    }

    return procedure;
}

void ti_procedure_destroy(ti_procedure_t * procedure)
{
    if (!procedure)
        return;

    ti_val_drop((ti_val_t *) procedure->name);
    vec_destroy(procedure->arguments, (vec_destroy_cb) ti_name_drop);
    vec_destroy(procedure->nd_val_cache, (vec_destroy_cb) ti_val_drop);

    if (procedure->node)
        cleri__node_free(procedure->node);
    free(procedure->body);
    free(procedure);
}

ti_procedure_t * ti_procedure_from_strn(const char * str, size_t n, ex_t * e)
{
    size_t count;
    cleri_parse_t * res = NULL;
    const char * start;
    const char * pt = str;
    ti_name_t * argname;
    ti_procedure_t * procedure = procedure__create();
    if (procedure)
        goto alloc_error;

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

    ++pt;
    --n;

    while(1)
    {
        procedure__skip_white_space();

        if (n && *pt == ')')
            break;

        if (!n || *pt != '$')
            goto no_tmp_argument;

        start = pt;

        if (!n || (!isalpha(*pt) && *pt != '_'))
            goto invalid_argument;

        ++pt;
        --n;

        for(count = 1; n && (isalnum(*pt) || *pt == '_'); ++count, ++pt, --n);

        argname = ti_names_get(start, count);
        if (!argname || vec_push(procedure->arguments, argname))
        {
            ti_name_drop(argname);
            goto alloc_error;
        }
    }

    ++pt;
    --n;

    procedure__skip_white_space();

    if (!n && *pt != '{')
        goto missing_body;

    ++pt;
    --n;

    procedure__skip_white_space();

    start = pt;

    pt += n;
    while (1)
    {
        if (pt <= start)
            goto missing_end;
        --pt;
        assert(n);  /* must be > 0 */
        --n;
        if (*pt == '}')
            break;
    }

    procedure->body = strndup(start, n);
    if (!procedure->body)
        goto alloc_error;


    res = cleri_parse2(
            ti()->langdef,
            procedure->body,
            CLERI_FLAG_EXPECTING_DISABLED|
            CLERI_FLAG_EXCLUDE_OPTIONAL);  /* only error position */
    if (!res)
        goto alloc_error;

    if (!res->is_valid)
    {
        ex_set(e, EX_BAD_DATA,
                "cannot create procedure: error in body at position `%zd`"
                TI_SEE_DOC("#new_procedure"),
                ((size_t) (start - str)) + res->pos);
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

    ti_syntax_investigate(&procedure->syntax, procedure->node);

    if (procedure->syntax.nd_val_cache_n)
    {
        procedure->nd_val_cache = vec_new(procedure->syntax.nd_val_cache_n);
        if (!procedure->nd_val_cache)
            goto alloc_error;

        ti_ncache_gen_primitives(procedure->nd_val_cache, procedure->node);
    }

    cleri_parse_free(res);
    return procedure;

invalid_name:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: invalid name at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            pt - str);
    goto failed;

missing_arguments:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: expecting `(` at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            pt - str);
    goto failed;

no_tmp_argument:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: "
            "argument at position `%d` should start with a `$`"
            TI_SEE_DOC("#new_procedure"),
            pt - str);
    goto failed;

invalid_argument:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: invalid argument name at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            pt - str);
    goto failed;

missing_body:
    ex_set(e, EX_BAD_DATA,
            "cannot create procedure: expecting `{` at position `%d`"
            TI_SEE_DOC("#new_procedure"),
            pt - str);
    goto failed;

missing_end:
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
    ti_procedure_destroy(procedure);
    return NULL;
}

