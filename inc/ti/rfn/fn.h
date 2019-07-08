#ifndef TI_CFN_FN_H_
#define TI_CFN_FN_H_

#include <assert.h>
#include <langdef/langdef.h>
#include <langdef/nd.h>
#include <stdlib.h>
#include <ti.h>
#include <ti/access.h>
#include <ti/auth.h>
#include <ti/collections.h>
#include <ti/nil.h>
#include <ti/opr.h>
#include <ti/regex.h>
#include <ti/rq.h>
#include <ti/task.h>
#include <ti/users.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/cryptx.h>
#include <util/strx.h>
#include <uv.h>

static _Bool rq__is_not_node(ti_query_t * q, cleri_node_t * n, ex_t * e);
static _Bool rq__is_not_thingsdb(ti_query_t * q, cleri_node_t * n, ex_t * e);

#endif  /* TI_CFN_FN_H_ */
