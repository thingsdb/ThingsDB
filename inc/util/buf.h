/*
 * util/buf.h
 */
#ifndef BUF_H_
#define BUF_H_

#include <stddef.h>

typedef struct buf_s buf_t;

void buf_init(buf_t * buf);
int buf_append(buf_t * buf, const char * s, size_t n);

#define buf_append_str(buf__, s__) \
        buf_append(buf__, s__, strlen(s__))

struct buf_s
{
    size_t cap;
    size_t len;
    char * data;
};

#endif  /* BUF_H_ */
