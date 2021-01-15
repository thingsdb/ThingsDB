/*
 * ti/str.h
 */
#ifndef TI_STR_H_
#define TI_STR_H_

typedef struct ti_str_s ti_str_t;

struct ti_str_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad0;
    uint32_t n;
    char str[];
};


#endif /* TI_STR_H_ */
