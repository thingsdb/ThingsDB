/*
 * ti/scope.c
 */
#include <assert.h>
#include <ctype.h>
#include <doc.h>
#include <ti.h>
#include <ti/name.h>
#include <ti/scope.h>
#include <ti/scope.h>
#include <tiinc.h>
#include <util/mpack.h>
#include <util/strx.h>

static _Bool scope__is_ascii(const char * str, size_t n, ex_t * e)
{
    if (!strx_is_asciin(str, n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid scope; "
                "scopes must only contain valid ASCII characters"DOC_SCOPES);
        return false;
    }
    return true;
}

static int64_t scope__read_id(const char * str, size_t n)
{
    uint64_t result = 0;

    assert (n);

    do
    {
        if (!isdigit(*str))
            return -1;

        result = 10 * result + *str - '0';

        if (result > (uint64_t) LLONG_MAX)
            return -1;

        ++str;
    }
    while (--n);

    return (int64_t) result;
}

static int scope__collection(
        ti_scope_t * scope,
        const char * str,
        size_t n,
        ex_t * e)
{
    assert (n);
    if (isdigit(*str))
    {
        int64_t collection_id = scope__read_id(str, n);
        if (collection_id < 0)
        {
            ex_set(e, EX_VALUE_ERROR,
                "invalid scope; "
                "a collection must be specified by either a name or id"
                DOC_SCOPES);
            return e->nr;
        }

        scope->tp = TI_SCOPE_COLLECTION_ID;
        scope->via.collection_id = (uint64_t) collection_id;

        return 0;  /* success */
    }

    if (!ti_name_is_valid_strn(str, n))
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid scope; "
            "the specified collection name is invalid"
            DOC_SCOPES);
        return e->nr;
    }

    scope->tp = TI_SCOPE_COLLECTION_NAME;
    scope->via.collection_name.name = str;
    scope->via.collection_name.sz = n;

    return 0;  /* success */
}

int ti_scope_init(ti_scope_t * scope, const char * str, size_t n, ex_t * e)
{
    char sep_char;

    if (n < 2)
        goto invalid_scope;

    switch(*str)
    {
    case '@': sep_char = ':'; break;
    case '/': sep_char = '/'; break;
    default:
        goto invalid_scope;
    }

    switch (str[1])
    {
    case 't':
        if (n > strlen("/thingsdb") || memcmp(str+1, "thingsdb", n-1))
            goto invalid_scope;

        scope->tp = TI_SCOPE_THINGSDB;
        return 0;    /* success */

    case 'n':
    {
        size_t i = 2;
        int64_t node_id;

        for (; i < n; ++i)
            if (str[i] == sep_char)
                break;

        if (i > strlen("/node") || memcmp(str+1, "node", i-1))
            goto invalid_scope;

        if (++i >= n)
        {
            scope->tp = TI_SCOPE_NODE;
            scope->via.node_id = ti.node->id;
            return 0;  /* success */
        }

        node_id = scope__read_id(str + i, n - i);

        if (node_id < 0 || node_id >= 0x40)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "invalid scope; "
                    "a node id must be an integer value between 0 and 64"
                    DOC_SCOPES);
            return e->nr;
        }

        scope->tp = TI_SCOPE_NODE;
        scope->via.node_id = (uint8_t) node_id;
        return 0;  /* success */
    }
    case 'c':
    {
        size_t i = 2;

        for (; i < n; ++i)
            if (str[i] == sep_char)
                break;

        if (i > strlen("/collection") || memcmp(str+1, "collection", i-1))
            goto invalid_scope;

        if (++i >= n)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "invalid scope; "
                    "the collection scope requires to specify a collection"
                    DOC_SCOPES);
            return e->nr;
        }
        return scope__collection(scope, str + i, n - i, e);

    }
    case '/':
    case ':':
        if (n < 3 || str[1] != sep_char)
            goto invalid_scope;

        return scope__collection(scope, str + 2, n - 2, e);
    }

    /* everything else is invalid */

invalid_scope:
    if (n == 0)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid scope; "
                "scopes must not be empty"DOC_SCOPES);
        return e->nr;
    }

    sep_char = *str == '@' ? ':' : *str == '/' ? '/' : '\0';

    if (!scope__is_ascii(str, n, e))
        return e->nr;

    if (!sep_char)
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid scope; "
            "scopes must start with a `@` or `/` but got `%.*s` instead"
            DOC_SCOPES,
            (int) n, str);

        return e->nr;
    }

    if (n == 1)
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid scope; "
            "expecting a scope name like "
            "`%c""thingsdb`, `%c""node%c""...` or `%c""collection%c""...`"
            DOC_SCOPES,
            *str, *str, sep_char, *str, sep_char);

        return e->nr;
    }

    ex_set(e, EX_VALUE_ERROR,
        "invalid scope; "
        "expecting a scope name like "
        "`%c""thingsdb`, `%c""node%c""...` or `%c""collection%c""...` "
        "but got `%.*s` instead"DOC_SCOPES,
        *str, *str, sep_char, *str, sep_char,
        (int) n, str);

    return e->nr;
}

/*
 * expecting the `data` to contain an array like: [scope, ...]
 */
int ti_scope_init_packed(
        ti_scope_t * scope,
        const unsigned char * data,
        size_t n,
        ex_t * e)
{
    mp_unp_t up;
    mp_obj_t obj, mp_scope;

    mp_unp_init(&up, data, n);

    if (mp_next(&up, &obj) != MP_ARR || !obj.via.sz ||
        mp_next(&up, &mp_scope) != MP_STR)
    {
        ex_set(e, EX_BAD_DATA,
                "expecting the request to contain an array "
                "with as scope as first item");
        return e->nr;
    }

    return ti_scope_init(scope, mp_scope.via.str.data, mp_scope.via.str.n, e);
}

int ti_scope_init_from_up(ti_scope_t * scope, mp_unp_t * up, ex_t * e)
{
    mp_obj_t obj, mp_scope;
    if (mp_next(up, &obj) != MP_ARR || !obj.via.sz ||
        mp_next(up, &mp_scope) != MP_STR)
    {
        ex_set(e, EX_BAD_DATA,
                "expecting the request to contain an array "
                "with as scope as first item");
        return e->nr;
    }

    return ti_scope_init(scope, mp_scope.via.str.data, mp_scope.via.str.n, e);

}


