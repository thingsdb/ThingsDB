/*
 * names.h
 */
#ifndef TI_NAMES_H_
#define TI_NAMES_H_

#include <stdint.h>
#include <ti/name.h>
#include <ti/val.h>
#include <ti.h>
#include <util/imap.h>

int ti_names_create(void);
void ti_names_destroy(void);
ti_name_t * ti_names_get(const char * str, size_t n);
ti_name_t * ti_names_weak_get(const char * str, size_t n);
static inline ti_name_t * ti_names_get_from_val(ti_val_t * val);


/* Returns name with reference or NULL in case of an error */
static inline ti_name_t * ti_names_get_from_val(ti_val_t * val)
{
    return val->tp == TI_VAL_RAW
            ? ti_names_get(
                (const char *) val->via.raw->data,
                val->via.raw->n)
            : ti_grab(val->via.name);
}

#endif /* TI_NAMES_H_ */
