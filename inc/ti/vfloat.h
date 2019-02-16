/*
 * ti/vfloat.h
 */
#ifndef TI_VFLOAT_H_
#define TI_VFLOAT_H_

typedef struct ti_vfloat_s ti_vfloat_t;

ti_vfloat_t * ti_vfloat_create(double d);
static inline void ti_vfloat_free(ti_vfloat_t * vfloat);

struct ti_vfloat_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad0;
    uint16_t _pad1;
    double float_;
};

static inline void ti_vint_free(ti_vfloat_t * vfloat)
{
    free(vfloat);
}

#endif  /* TI_VFLOAT_H_ */
