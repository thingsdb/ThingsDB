/*
 * ti/vattr.h
 */
#ifndef TI_VATTR_H_
#define TI_VATTR_H_

typedef struct ti_vattr_s ti_vattr_t;

ti_vattr_t * ti_vattr_create(void);

struct ti_vattr_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad0;
    uint16_t _pad1;
    ti_prop_t * prop;
};

static inline void ti_vint_free(ti_vint_t * vint)
{
    free(vint);
}

#endif  /* TI_VATTR_H_ */
