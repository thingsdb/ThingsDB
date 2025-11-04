/*
 * ti/commits.c
 */
#include <ti/commits.h>
#include <ti/val.inline.h>
#include <ti/scope.h>
#include <ti.h>

typedef struct
{
    ti_commits_options_t * options;
    ex_t * e;
    _Bool allow_detail;
} commits__options_t;

static int commits__options(
        ti_raw_t * key,
        ti_val_t * val,
        commits__options_t * w)
{
    if (ti_raw_eq_strn(key, "scope", strlen("scope")))
    {
        if (!ti_val_is_str(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `scope` to be of type `"TI_VAL_STR_S"` but "
                "got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->scope = (ti_raw_t *) val;
        return 0;
    }
    if (ti_raw_eq_strn(key, "contains", strlen("contains")))
    {
        if (!ti_val_is_str(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `contains` to be of type `"TI_VAL_STR_S"` but "
                "got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->contains = (ti_raw_t *) val;
        return 0;
    }
    if (ti_raw_eq_strn(key, "match", strlen("match")))
    {
        if (!ti_val_is_regex(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `match` to be of type `"TI_VAL_REGEX_S"` but "
                "got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->match = (ti_regex_t *) val;
        return 0;
    }
    if (ti_raw_eq_strn(key, "id", strlen("id")))
    {
        if (!ti_val_is_int(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `id` to be of type `"TI_VAL_INT_S"` but "
                "got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->id = (ti_vint_t *) val;
        return 0;
    }
    if (ti_raw_eq_strn(key, "last", strlen("last")))
    {
        if (!ti_val_is_int(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `last` to be of type `"TI_VAL_INT_S"` but "
                "got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->last = (ti_vint_t *) val;
        return 0;
    }
    if (ti_raw_eq_strn(key, "first", strlen("first")))
    {
        if (!ti_val_is_int(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `first` to be of type `"TI_VAL_INT_S"` but "
                "got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->first = (ti_vint_t *) val;
        return 0;
    }
    if (ti_raw_eq_strn(key, "before", strlen("before")))
    {
        if (!ti_val_is_datetime(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `before` to be of "
                "type `"TI_VAL_DATETIME_S"` or `"TI_VAL_TIMEVAL_S"` "
                "but got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->before = (ti_datetime_t *) val;
        return 0;
    }
    if (ti_raw_eq_strn(key, "after", strlen("after")))
    {
        if (!ti_val_is_datetime(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `after` to be of "
                "type `"TI_VAL_DATETIME_S"` or `"TI_VAL_TIMEVAL_S"` "
                "but got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->after = (ti_datetime_t *) val;
        return 0;
    }
    if (w->allow_detail && ti_raw_eq_strn(key, "detail", strlen("detail")))
    {
        if (!ti_val_is_bool(val))
        {
            ex_set(w->e, EX_TYPE_ERROR,
                "expecting `detail` to be of type `"TI_VAL_BOOL_S"` but "
                "got type `%s` instead",
                ti_val_str(val));
            return w->e->nr;
        }
        w->options->detail = (ti_vbool_t *) val;
        return 0;
    }
    ex_set(w->e, EX_VALUE_ERROR,
        "invalid option `%.*s`; valid options: `scope`, `contains`, "
        "`match`, `id`, `first`, `last`, `before`, `after`%s",
        key->n, key->data,
        w->allow_detail ? ", `detail`" : "");
    return 0;
}

int ti_commits_options(
    ti_commits_options_t * options,
    ti_thing_t * thing,
    _Bool allow_detail,
    ex_t * e)
{
    commits__options_t w = {
        .options=options,
        .e=e,
        .allow_detail=allow_detail,
    };

    if (ti_thing_walk(thing, (ti_thing_item_cb) commits__options, &w))
        return e->nr;  /* error is set */

    if (!options->contains &&
        !options->match &&
        !options->id &&
        !options->first &&
        !options->last &&
        !options->before &&
        !options->after)
        ex_set(e, EX_LOOKUP_ERROR,
            "at least one of the following options must be set: "
            "`contains`, `match`, `id`, `first`, `last`, `before`, `after`");
    return e->nr;
}

int ti_commits_history(
    ti_commits_history_t * history,
    ti_commits_options_t * options,
    ex_t * e)
 {
    history->commits = &ti.commits;
    history->access = &ti.access_thingsdb;
    history->scope_id = 0;

    if (options->scope)
    {
        ti_scope_t scope;
        if (ti_scope_init(
                &scope,
                (const char *) options->scope->data,
                options->scope->n,
                e))
            return e->nr;

        switch(scope.tp)
        {
            case TI_SCOPE_THINGSDB:
                history->commits = &ti.commits;
                history->access = &ti.access_thingsdb;
                break;
            case TI_SCOPE_NODE:
                history->commits = NULL;
                history->access = NULL;
                break;
            case TI_SCOPE_COLLECTION:
            {
                ti_collection_t * collection = ti_collections_get_by_strn(
                        scope.via.collection_name.name,
                        scope.via.collection_name.sz);
                if (!collection)
                {
                    ex_set(e, EX_LOOKUP_ERROR,
                        "collection `%.*s` not found",
                        scope.via.collection_name.sz,
                        scope.via.collection_name.name);
                    return e->nr;
                }
                history->commits = &collection->commits;
                history->access = &collection->access;
                history->scope_id = collection->id;
                break;
            }
        }
        if (!(*history->commits))
            ex_set(e, EX_OPERATION,
                "history is not enabled for the `*.*s` scope",
                options->scope->n,
                options->scope->data);
        return e->nr;
    }

    for (vec_each(ti.collections->vec, ti_collection_t, collection))
    {
        if (collection->commits)
        {
            if (*history->commits)
            {
                ex_set(e, EX_LOOKUP_ERROR,
                    "multiple scopes with `history` enabled; "
                    "use the `scope` option to specify an explicit scope");
                return e->nr;
            }
            history->commits = &collection->commits;
            history->access = &collection->access;
            history->scope_id = collection->id;
        }
    }

    if (!(*history->commits))
        ex_set(e, EX_LOOKUP_ERROR,
            "no scope found with `history` enabled"DOC_SET_HISTORY);
    return e->nr;
}

vec_t ** ti_commits_from_scope(ti_raw_t * scope, ex_t * e)
{
    ti_scope_t scope_;
    if (ti_scope_init(&scope_, (const char *) scope->data, scope->n, e))
        return NULL;

    switch(scope_.tp)
    {
    case TI_SCOPE_THINGSDB:
        return &ti.commits;
    case TI_SCOPE_NODE:
        return NULL;
    case TI_SCOPE_COLLECTION:
    {
        ti_collection_t * collection = ti_collections_get_by_strn(
                scope_.via.collection_name.name,
                scope_.via.collection_name.sz);
        if (collection)
            return &collection->commits;

        ex_set(e, EX_LOOKUP_ERROR, "collection `%.*s` not found",
                scope_.via.collection_name.sz,
                scope_.via.collection_name.name);
    }
    }
    return NULL;
}

void ti_commits_destroy(vec_t ** commits)
{
    vec_destroy(*commits, (vec_destroy_cb) ti_commit_destroy);
    *commits = NULL;
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
    ti_commits_destroy(commits);
    return 0;
}

static _Bool commits__match(
    ti_commit_t * commit,
    ti_commits_options_t * options)
{
    if (options->contains && (
            !ti_raw_contains(commit->code, options->contains) &&
            !ti_raw_contains(commit->message, options->contains) &&
            !ti_raw_contains(commit->by, options->contains) && (
                !commit->err_msg ||
                !ti_raw_contains(commit->err_msg, options->contains)
            )
        ))
        return false;
    if (options->match && (
            !ti_regex_test(options->match, commit->code) &&
            !ti_regex_test(options->match, commit->message) &&
            !ti_regex_test(options->match, commit->by) && (
                !commit->err_msg ||
                !ti_regex_test(options->match, commit->err_msg)
            )
        ))
        return false;
    if (options->id && commit->id != (uint64_t) VINT(options->id))
        return false;

    return true;
}

vec_t * ti_commits_find(vec_t * commits, ti_commits_options_t * options)
{
    vec_t * vec = vec_new(8);
    if (!vec)
        return NULL;
    for (vec_each(commits, ti_commit_t, commit))
        if (commits__match(commit, options) && vec_push(&vec, commit))
            goto fail;

    return vec;
fail:
    vec_destroy(vec, NULL);
    return NULL;
}

vec_t * ti_commits_del(vec_t ** commits, ti_commits_options_t * options)
{
    vec_t * vec = vec_new(8);
    uint32_t n = (*commits)->n;
    if (!vec)
        return NULL;

    for (vec_each_rev(*commits, ti_commit_t, commit) --n)
        if (commits__match(commit, options) &&
            vec_push(&vec, vec_remove(*commits, n)))
            goto fail;

    (void) vec_shrink(commits);
    return vec;
fail:
    /* revert */
    for (vec_each(vec, ti_commit_t, commit))
        VEC_push(*commits, commit);
    vec_destroy(vec, NULL);
    return NULL;
}
