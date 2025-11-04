/*
 * ti/commits.h
 */
#ifndef TI_COMMITS_H_
#define TI_COMMITS_H_

#include <ti/datetime.h>
#include <ti/raw.t.h>
#include <ti/regex.t.h>
#include <ti/thing.t.h>
#include <ti/varr.t.h>
#include <ti/vbool.h>
#include <ti/vint.h>
#include <util/vec.h>
#include <ex.h>

typedef struct
{
    ti_raw_t * scope;
    ti_raw_t * contains;
    ti_regex_t * match;
    ti_vint_t * id;
    ti_vint_t * last;
    ti_vint_t * first;
    ti_datetime_t * before;
    ti_datetime_t * after;
    ti_vbool_t * detail;
} ti_commits_options_t;

typedef struct
{
    vec_t ** commits;
    vec_t ** access;
    uint64_t scope_id;
} ti_commits_history_t;

void ti_commits_destroy(vec_t ** commits);
int ti_commits_options(
    ti_commits_options_t * options,
    ti_thing_t * thing,
    _Bool allow_detail,
    ex_t * e);
int ti_commits_history(
    ti_commits_history_t * history,
    ti_commits_options_t * options,
    ex_t * e);
vec_t ** ti_commits_from_scope(ti_raw_t * scope, ex_t * e);
int ti_commits_set_history(vec_t ** commits, _Bool state);
vec_t * ti_commits_find(vec_t * commits, ti_commits_options_t * options);
vec_t * ti_commits_del(vec_t ** commits, ti_commits_options_t * options);

#endif /* TI_COMMITS_H_ */
