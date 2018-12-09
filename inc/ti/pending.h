/*
 * ti/pending.h
 */
#ifndef TI_PENDING_H_
#define TI_PENDING_H_

typedef struct ti_pending_s ti_pending_t;

#include <uv.h>

int ti_pending_create(void);
void ti_pending_destroy(void);
int ti_pendging_from_strn(const char * str, size_t n);

struct ti_pending_s
{
    struct sockaddr_storage addr;
    uint16_t port;
    uint8_t node_id;
};

#endif /* TI_PENDING_H_ */
