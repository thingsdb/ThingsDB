/*
 * ref.h
 */
#ifndef TI_REF_H_
#define TI_REF_H_

typedef struct ti_ref_s  ti_ref_t;

#include <assert.h>
#include <stdint.h>

struct ti_ref_s
{
    uint32_t ref;
};

static inline void ti_ref_inc(ti_ref_t * ref);
static inline void ti_ref_dec(ti_ref_t * ref);

/* macro for ti_ref_inc() */
#define TI_ref_inc(ref__) (ref__)->ref++

/* unsafe macro for ti_ref_dec() which assumes more than one reference
 * is left */
#define TI_ref_dec(ref__) (ref__)->ref--


static inline void ti_ref_inc(ti_ref_t * ref)
{
    ref->ref++;
}

static inline void ti_ref_dec(ti_ref_t * ref)
{
    assert(ref->ref > 1);
    ref->ref--;
}

#endif /* TI_REF_H_ */
