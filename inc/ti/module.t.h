/*
 * ti/module.t.h
 */
#ifndef TI_MODULE_T_H_
#define TI_MODULE_T_H_

typedef struct ti_module_s ti_module_t;
typedef void (*ti_module_cb)(void * future);

#include <inttypes.h>
#include <ti/proc.t.h>
#include <ti/name.t.h>
#include <ti/pkg.t.h>
#include <util/omap.h>

enum
{
    /* negative values are reserved for uv errors */
    TI_MODULE_STAT_RUNNING,         /* success */
    TI_MODULE_STAT_NOT_LOADED,
    TI_MODULE_STAT_STOP_AND_DESTROY,
    TI_MODULE_STAT_TOO_MANY_RESTARTS,
};

struct ti_module_s
{
    int status;             /* 0 = success, >0 = enum, <0 = uv error */
    uint16_t restarts;      /* keep the number of times this module has been
                               restarted */
    uint16_t next_pid;      /* next package id  */
    ti_module_cb cb;        /* module callback */
    ti_name_t * name;       /* name of the module */
    char * file;            /* file (full path) to start */
    char * fn;              /* just the file name (using a pointer to file) */
    char ** args;
    ti_pkg_t * conf_pkg;    /* configuration package */
    uint64_t started_at;    /* module started at this time-stamp */
    uint64_t created_at;    /* module started at this time-stamp */
    uint64_t * scope_id;    /* may be NULL */
    omap_t * futures;       /* ti_future_t (no reference, parent query holds
                               a reference so no extra is needed) */
    ti_proc_t proc;
};


#endif /* TI_MODULE_T_H_ */
