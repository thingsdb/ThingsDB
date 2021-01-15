/*
 * ti/module.t.h
 */
#ifndef TI_MODULE_T_H_
#define TI_MODULE_T_H_

typedef struct ti_module_s ti_module_t;
typedef void (*ti_module_cb)(void * future);

#include <inttypes.h>
#include <ti/proc.t.h>

enum
{
    /* negative values are reserved for uv errors */
    TI_MODULE_STAT_RUNNING,         /* success */
    TI_MODULE_STAT_NOT_LOADED,
    TI_MODULE_STAT_STOPPING,
    TI_MODULE_STAT_TOO_MANY_RESTARTS,
};

struct ti_module_s
{
    int status;             /* 0 = success, >0 = enum, <0 = uv error */
    uint16_t restarts;      /* keep the number of times this module has been
                               restarted */
    uint16_t next_pid;      /* next package id  */
    ti_module_cb cb;        /* module callback */
    ti_raw_t * name;        /* name of the module */
    ti_name_t * binary;     /* binary to start */
    ti_pkg_t * conf_pkg;    /* configuration package */
    uint64_t started_at;    /* module started at this time-stamp */
    uint64_t created_at;    /* module started at this time-stamp */
    ti_scope_t * scope;     /* may be NULL */
    omap_t * futures;       /* ti_future_t (no reference, parent query holds
                               a reference so no extra is needed) */
    ti_proc_t proc;
};


#endif /* TI_MODULE_T_H_ */
