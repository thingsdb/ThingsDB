/*
 * ti/ano.h
 */
#ifndef TI_ANO_H_
#define TI_ANO_H_

#include <ti/ano.t.h>
#include <ti/val.t.h>
#include <ti/collection.t.h>
#include <util/mpack.h>
#include <ex.h>

#define ANO_T(__x) ((ti_ano_t *) (__x))->type

ti_ano_t * ti_ano_new(void);
int ti_ano_init(
        ti_ano_t * ano,
        ti_collection_t * collection,
        ti_raw_t * spec_raw,
        ex_t * e);
ti_ano_t * ti_ano_from_raw(
        ti_collection_t * collection,
        ti_raw_t * spec_raw,
        ex_t * e);
ti_ano_t * ti_ano_create(
        ti_collection_t * collection,
        const unsigned char * bin,
        size_t n,
        ex_t * e);
void ti_ano_destroy(ti_ano_t * ano);

static inline int ti_ano_to_client_pk(
        ti_ano_t * ano,
        msgpack_packer * pk)
{
    return mp_pack_append(pk, ano->spec_raw->data, ano->spec_raw->n);
}

static inline _Bool ti_ano_uninitialized(ti_ano_t * ano)
{
    return !ano->spec_raw;
}

static inline int ti_ano_to_store_pk(
        ti_ano_t * ano,
        msgpack_packer * pk)
{
    return mp_pack_ext(
        pk,
        MPACK_EXT_ANO,
        ano->spec_raw->data,
        ano->spec_raw->n);
}

#endif /* TI_ANO_H_ */
