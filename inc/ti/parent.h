/*
 * ti/parent.h
 */
#ifndef TI_PARENT_H_
#define TI_PARENT_H_

typedef struct
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    int:16;
    ti_thing_t * parent;    /* without reference, NULL when a variable */
    void * key_;    /* ti_name_t, ti_raw_t or ti_field_t; without reference */
} ti_parent_t;


#endif /* TI_PARENT_H_ */
