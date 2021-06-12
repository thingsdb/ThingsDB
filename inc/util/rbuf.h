/*
 * util/rbuf.h
 */
#ifndef RBUF_H_
#define RBUF_H_

#include <stddef.h>
#include <string.h>

typedef struct rbuf_s rbuf_t;

void rbuf_init(rbuf_t * buf);
int rbuf_append(rbuf_t * buf, const char * s, size_t n);
int rbuf_write(rbuf_t * buf, const char c);

struct rbuf_s
{
    size_t pos;
    size_t cap;
    char * data;
};

#define rbuf_len(__rbuf) ((__rbuf)->cap - (__rbuf)->pos)

#endif  /* RBUF_H_ */
