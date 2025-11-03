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

int ti_commits_set_history(vec_t ** commits, _Bool state);
ti_varr_t * ti_commits_find(vec_t ** commits, ti_thing_t * thing);
vec_t * ti_commits_del(vec_t ** commits, ti_thing_t * thing);

#endif /* TI_COMMITS_H_ */
