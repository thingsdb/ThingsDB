/*
 * ti/task.t.h
 */
#ifndef TI_TASK_T_H_
#define TI_TASK_T_H_

/*
 * !! IMPORTANT !!
 * Always keep the same number assigned to a task!
 */
typedef enum
{
    TI_TASK_SET,                            /* 0   */
    TI_TASK_DEL,                            /* 1   */
    TI_TASK_SPLICE,                         /* 2   */
    TI_TASK_SET_ADD,                        /* 3   */
    TI_TASK_SET_REMOVE,                     /* 4   */
    TI_TASK_DEL_COLLECTION,                 /* 5   */
    TI_TASK_DEL_ENUM,                       /* 6   */
    TI_TASK_DEL_EXPIRED,                    /* 7   */
    TI_TASK_DEL_MODULE,                     /* 8   */
    TI_TASK_DEL_NODE,                       /* 9  */
    TI_TASK_DEL_PROCEDURE,                  /* 10  */
    TI_TASK_DEL_TIMER,                      /* 11  */
    TI_TASK_DEL_TOKEN,                      /* 12  */
    TI_TASK_DEL_TYPE,                       /* 13  */
    TI_TASK_DEL_USER,                       /* 14  */
    TI_TASK_DEPLOY_MODULE,                  /* 15  */
    TI_TASK_GRANT,                          /* 16  */
    TI_TASK_MOD_ENUM_ADD,                   /* 17  */
    TI_TASK_MOD_ENUM_DEF,                   /* 18  */
    TI_TASK_MOD_ENUM_DEL,                   /* 19  */
    TI_TASK_MOD_ENUM_MOD,                   /* 20  */
    TI_TASK_MOD_ENUM_REN,                   /* 21  */
    TI_TASK_MOD_TYPE_ADD,                   /* 22  */
    TI_TASK_MOD_TYPE_DEL,                   /* 23  */
    TI_TASK_MOD_TYPE_MOD,                   /* 24  */
    TI_TASK_MOD_TYPE_REL_ADD,               /* 25  */
    TI_TASK_MOD_TYPE_REL_DEL,               /* 26  */
    TI_TASK_MOD_TYPE_REN,                   /* 27  */
    TI_TASK_MOD_TYPE_WPO,                   /* 28  */
    TI_TASK_NEW_COLLECTION,                 /* 29  */
    TI_TASK_NEW_MODULE,                     /* 30  */
    TI_TASK_NEW_NODE,                       /* 31  */
    TI_TASK_NEW_PROCEDURE,                  /* 32  */
    TI_TASK_NEW_TIMER,                      /* 33  */
    TI_TASK_NEW_TOKEN,                      /* 34  */
    TI_TASK_NEW_TYPE,                       /* 35  */
    TI_TASK_NEW_USER,                       /* 36  */
    TI_TASK_RENAME_COLLECTION,              /* 37  */
    TI_TASK_RENAME_ENUM,                    /* 38  */
    TI_TASK_RENAME_MODULE,                  /* 39  */
    TI_TASK_RENAME_PROCEDURE,               /* 40  */
    TI_TASK_RENAME_TYPE,                    /* 41  */
    TI_TASK_RENAME_USER,                    /* 42  */
    TI_TASK_RESTORE,                        /* 43  */
    TI_TASK_REVOKE,                         /* 44  */
    TI_TASK_SET_ENUM,                       /* 45  */
    TI_TASK_SET_MODULE_CONF,                /* 46  */
    TI_TASK_SET_MODULE_SCOPE,               /* 47  */
    TI_TASK_SET_PASSWORD,                   /* 48  */
    TI_TASK_SET_TIME_ZONE,                  /* 49  */
    TI_TASK_SET_TIMER_ARGS,                 /* 50  */
    TI_TASK_SET_TYPE,                       /* 51  */
    TI_TASK_TO_TYPE,                        /* 52  */
    TI_TASK_CLEAR_USERS,                    /* 53  */
    TI_TASK_TAKE_ACCESS,                    /* 54  */

} ti_task_enum;

typedef struct ti_task_s ti_task_t;

#include <inttypes.h>
#include <stdlib.h>
#include <ti/thing.t.h>
#include <util/vec.h>

struct ti_task_s
{
    uint64_t event_id;
    size_t approx_sz;
    ti_thing_t * thing;     /* with reference */
    vec_t * jobs;           /* q-pack (unsigned char *) */
};

#endif /* TI_TASK_T_H_ */
