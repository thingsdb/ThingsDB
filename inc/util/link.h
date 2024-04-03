/*
 * link.h
 */
#ifndef LINK_H_
#define LINK_H_

typedef struct link_s  link_t;
typedef struct link__s ** link_iter_t;

typedef void (*link_destroy_cb)(void * data);

/* private */
struct link__s
{
    struct link__s * next_;
    void * data_;
};

link_t * link_create(void);
void link_destroy(link_t * link, link_destroy_cb cb);

void link_clear(link_t * link, link_destroy_cb cb);
void * link_rm(link_t * link, void * data);
int link_insert(link_t * link, void * data);
void * link_fpop(link_t * link);  /* remove the first */

/* loop using the iterator */
#define link_each(iter__, dt__, var__) \
        dt__ * var__; \
        *iter__ && \
        (var__ = (dt__ *) (*iter__)->data_); \
        iter__ = &(*iter__)->next_

struct link_s
{
    struct link__s * next_;
    size_t n;
};

/* iterator functions */
static inline link_iter_t link_iter(link_t * link)
{
    return &link->next_;
}

static inline void * link_first(link_t * link)
{
    return link->next_ ? link->next_->data_ : NULL;
}

#endif /* LINK_H_ */
