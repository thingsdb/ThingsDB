/*
 * ti/scope.c
 */
#include <ti/scope.h>
#include <qpack.h>
#include <tiinc.h>
#include <ctype.h>
#include <util/strx.h>
#include <assert.h>
#include <ti/name.h>
#include <ti.h>

#define SCOPE__THINGSDB "@thingsdb"
#define SCOPE__NODE "@node"
#define SCOPE__COLLECTION "@collection"

static _Bool scope__is_ascii(const char * str, size_t n, ex_t * e)
{
    if (!strx_is_asciin(str, n))
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid scope; "
                "scopes must only contain valid ASCII characters"SCOPES_DOC_);
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
                SCOPES_DOC_);
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
            SCOPES_DOC_);
        return e->nr;
    }

    scope->tp = TI_SCOPE_COLLECTION_NAME;
    scope->via.collection_name.name = str;
    scope->via.collection_name.sz = n;

    return 0;  /* success */
}

int ti_scope_init(ti_scope_t * scope, const char * str, size_t n, ex_t * e)
{
    if (n < 2)
        goto invalid_scope;

    switch (str[1])
    {
    case 't':
        if (n > strlen(SCOPE__THINGSDB) || memcmp(str, SCOPE__THINGSDB, n))
            goto invalid_scope;

        scope->tp = TI_SCOPE_THINGSDB;
        return 0;    /* success */

    case 'n':
    {
        size_t i = 2;
        int64_t node_id;

        for (; i < n; ++i)
            if (str[i] == ':')
                break;

        if (i > strlen(SCOPE__NODE) || memcmp(str, SCOPE__NODE, i))
            goto invalid_scope;

        if (++i >= n)
        {
            scope->tp = TI_SCOPE_NODE;
            scope->via.node_id = ti()->node->id;
            return 0;  /* success */
        }

        node_id = scope__read_id(str + i, n - i);

        if (node_id < 0 || node_id >= 0x40)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "invalid scope; "
                    "a node id must be an integer value between 0 and 64"
                    SCOPES_DOC_);
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
            if (str[i] == ':')
                break;

        if (i > strlen(SCOPE__COLLECTION) ||
            memcmp(str, SCOPE__COLLECTION, i))
            goto invalid_scope;

        if (++i >= n)
        {
            ex_set(e, EX_VALUE_ERROR,
                    "invalid scope; "
                    "the collection scope requires to specify a collection"
                    SCOPES_DOC_);
            return e->nr;
        }
        return scope__collection(scope, str + i, n - i, e);

    }
    case ':':
        if (n < 3 || *str != '@')
            goto invalid_scope;

        return scope__collection(scope, str + 2, n - 2, e);
    }

    /* everything else is invalid */

invalid_scope:
    if (n == 0)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid scope; "
                "scopes must not be empty"SCOPES_DOC_);
        return e->nr;
    }

    if (!scope__is_ascii(str, n, e))
        return e->nr;

    if (*str != '@')
    {

        ex_set(e, EX_VALUE_ERROR,
            "invalid scope; "
            "scopes must start with a `@` but got `%.*s` instead"SCOPES_DOC_,
            (int) n, str);

        return e->nr;
    }

    if (n == 1)
    {
        ex_set(e, EX_VALUE_ERROR,
            "invalid scope; "
            "expecting a scope name like "
            "`@thingsdb`, `@node:...` or `@collection:...`"SCOPES_DOC_);

        return e->nr;
    }

    ex_set(e, EX_VALUE_ERROR,
        "invalid scope; "
        "expecting a scope name like "
        "`@thingsdb`, `@node:...` or `@collection:...` "
        "but got `%.*s` instead"SCOPES_DOC_,
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
    qp_unpacker_t unpacker;
    qp_obj_t qp_s;

    qp_unpacker_init(&unpacker, data, n);

    if (!qp_is_array(qp_next(&unpacker, NULL)))
    {
        ex_set(e, EX_BAD_DATA, "expecting the request to contain an array");
        return e->nr;
    }

    if (!qp_is_raw(qp_next(&unpacker, &qp_s)))
    {
        ex_set(e, EX_TYPE_ERROR,
                "expecting the first item in the array request to be of of "
                "type string");
        return e->nr;
    }

    return ti_scope_init(scope, (const char *) qp_s.via.raw, qp_s.len, e);
}


