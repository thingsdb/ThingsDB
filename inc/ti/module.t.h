/*
 * ti/module.t.h
 */
#ifndef TI_MODULE_T_H_
#define TI_MODULE_T_H_

typedef struct ti_module_s ti_module_t;
typedef void (*ti_module_cb)(void * future);

#define TI_MODULE_MAX_ERR 255
#define TI_MODULE_DEFAULT_LOAD false
#define TI_MODULE_DEFAULT_DEEP 1

#include <inttypes.h>
#include <ti/mod/github.t.h>
#include <ti/mod/manifest.t.h>
#include <ti/name.t.h>
#include <ti/pkg.t.h>
#include <ti/proc.t.h>
#include <util/omap.h>

typedef enum
{
    /* negative values are reserved for uv errors */
    TI_MODULE_STAT_RUNNING,                 /* success */
    TI_MODULE_STAT_NOT_INSTALLED,
    TI_MODULE_STAT_INSTALLER_BUSY,
    TI_MODULE_STAT_NOT_LOADED,
    TI_MODULE_STAT_STOPPING,
    TI_MODULE_STAT_TOO_MANY_RESTARTS,
    TI_MODULE_STAT_PY_INTERPRETER_NOT_FOUND,
    TI_MODULE_STAT_CONFIGURATION_ERR,
    TI_MODULE_STAT_SOURCE_ERR,
    TI_MODULE_STAT_INSTALL_FAILED,
} ti_module_stat_t;

enum
{
    TI_MODULE_FLAG_IN_USE           =1<<0,
    TI_MODULE_FLAG_WAIT_CONF        =1<<1,
    TI_MODULE_FLAG_DESTROY          =1<<2,
    TI_MODULE_FLAG_RESTARTING       =1<<3,
    TI_MODULE_FLAG_WITH_CONF        =1<<4,      /* used for info */
    TI_MODULE_FLAG_WITH_TASKS       =1<<5,      /* used for info */
    TI_MODULE_FLAG_WITH_RESTARTS    =1<<6,      /* used for info */
    TI_MODULE_FLAG_DEL_FILES        =1<<7,
    TI_MODULE_FLAG_UPDATE           =1<<8,
};

typedef enum
{
    TI_MODULE_SOURCE_FILE,
    TI_MODULE_SOURCE_GITHUB
} ti_module_source_enum_t;

typedef union
{
    char * file;                    /* just a pointer to module->orig */
    ti_mod_github_t * github;       /* unpacked GitHub module */
} ti_module_source_via_t;

struct ti_module_s
{
    uint32_t ref;
    uint8_t tp;
    long:24;

    int status;             /* 0 = success, >0 = enum, <0 = uv error */
    int flags;
    uint16_t restarts;      /* keep the number of times this module has been
                               restarted */
    uint16_t next_pid;      /* next package id  */
    ti_module_cb cb;        /* module callback */
    ti_name_t * name;       /* name of the module */
    char * orig;            /* original source of the module */
    char * path;            /* path with the module, without tailing `/` */
    char * file;            /* file (full path) to start */
    char * fn;              /* just the file name (using a pointer to file) */
    char ** args;           /* process arguments (main file etc.) */
    ti_pkg_t * conf_pkg;    /* configuration package */
    uint64_t started_at;    /* module started at this time-stamp */
    uint64_t created_at;    /* module started at this time-stamp */
    uint64_t * scope_id;    /* bound to a scope, may be NULL for all scopes */
    omap_t * futures;       /* ti_future_t (no reference, parent query holds
                               a reference so no extra is needed) */
    ti_mod_manifest_t manifest;             /* manifest from module.json */
    ti_proc_t proc;                         /* process */
    ti_module_source_enum_t source_type;    /* source type: file/GitHub/.. */
    ti_module_source_via_t source;          /* source */
    char source_err[TI_MODULE_MAX_ERR];     /* error message from source;
                                               the source_err will only be
                                               accessed by the main thread when
                                               the module is *not* installing,
                                               thus not being written by
                                               an installation thread;  */
    uv_timer_t * wait_deploy;
};


#endif /* TI_MODULE_T_H_ */
