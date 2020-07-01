/*
 * ti/vbool.h
 */
#ifndef TI_VBOOL_H_
#define TI_VBOOL_H_

typedef struct ti_vbool_s ti_vbool_t;

#define VBOOL(__x)  ((ti_vbool_t *) (__x))->bool_

#include <inttypes.h>

struct ti_vbool_s
{
    uint32_t ref;
    uint8_t tp;
    _Bool bool_;
    uint16_t _pad16;
};

ti_vbool_t * ti_vbool_get(_Bool b);
_Bool ti_vbool_no_ref(void);

#endif  /* TI_VBOOL_H_ */
