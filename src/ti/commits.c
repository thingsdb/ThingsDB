/*
 * ti/commits.c
 */
#include <ti/commits.h>
#include <ti.h>

typedef struct
{
    ti_raw_t * scope;
    ti_raw_t * contains;
    ti_regex_t * match;
    ti_vint_t * id;
    ti_vint_t * last;
    ti_vint_t * first;
    ti_vbool_t * detail;
    ti_datetime_t * before;
    ti_datetime_t * after;
    ex_t * e;
} commits__options_t;

vec_t ** ti_commits_from_scope(ti_raw_t * scope, ex_t * e)
{
    if (ti_scope_init(&scope, (const char *) scope->data, scope->n, e))
        return NULL;

    switch(scope.tp)
    {
    case TI_SCOPE_THINGSDB:
        return &ti.commits;
    case TI_SCOPE_NODE:
        return NULL;
    case TI_SCOPE_COLLECTION:
        collection = ti_collections_get_by_strn(
                scope.via.collection_name.name,
                scope.via.collection_name.sz);
        if (collection)
            return &collection->access;

        ex_set(e, EX_LOOKUP_ERROR, "collection `%.*s` not found",
                scope.via.collection_name.sz,
                scope.via.collection_name.name);
    }
    return NULL;
}

int ti_commits_set_history(vec_t ** commits, _Bool state)
{
    if (state)
    {
        if (*commits)
            return 0;

        *commits = vec_new(8);
        return !!(*commits);
    }
    vec_destroy(*commits, ti_commit_destroy);
    *commits = NULL;
    return 0;
}