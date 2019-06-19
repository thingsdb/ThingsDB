#ifndef TI_OPR_OPRINC_H_
#define TI_OPR_OPRINC_H_

#define CAST_MAX 9223372036854775808.0

#define OPR__BOOL(__x)  ((ti_vbool_t *) __x)->bool_
#define OPR__INT(__x)   ((ti_vint_t *) __x)->int_
#define OPR__FLOAT(__x) ((ti_vfloat_t *) __x)->float_
#define OPR__RAW(__x)   ((ti_raw_t *) __x)

#include <assert.h>
#include <ti/opr.h>
#include <ti/raw.h>
#include <ti/vbool.h>
#include <ti/vint.h>
#include <ti/vfloat.h>
#include <ti/vset.h>
#include <util/logger.h>

#endif  /* TI_OPR_OPRINC_H_ */
