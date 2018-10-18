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
    void * data_;
    struct link__s * next_;
};

link_t * link_create(void);
void link_destroy(link_t * link, link_destroy_cb cb);
int link_insert(link_t * link, size_t idx, void * data);
void * link_pop(link_t * link);
static inline int link_push(link_t * link, void * data);
static inline int link_unshift(link_t * link, void * data);
static inline void * link_shift(link_t * link);

/* iterator functions */
static inline link_iter_t link_iter(link_t * link);
void link_iter_remove(link_t * link, link_iter_t iter);
int link_iter_insert(link_t * link, link_iter_t iter, void * data);
#define link_iter_next(iter__) (iter__) = &(*(iter__)) ? &(*(iter__))->next_ : (iter__)
static inline void * link_iter_get(link_iter_t iter);

/* loop using the iterator */
#define link_each(iter__, dt__, var__) \
        dt__ * var__; \
        *iter__ && \
        (var__ = (dt__ *) (*iter__)->data_); \
        iter__ = &(*iter__)->next_

/* private function */
void * link__pop__(link_t * link, struct link__s ** link_);

struct link_s
{
    size_t n;
    struct link__s * next_;
};

static inline int link_push(link_t * link, void * data)
{
    return link_insert(link, link->n, data);
}

static inline int link_unshift(link_t * link, void * data)
{
    return link_insert(link, 0, data);
}

static inline void * link_shift(link_t * link)
{
    return (link->next_) ? link__pop__(link, &link->next_) : NULL;
}

static inline link_iter_t link_iter(link_t * link)
{
    return &link->next_;
}

static inline void * link_iter_get(link_iter_t iter)
{
    return (*iter) ? (*iter)->data_ : NULL;
}


#endif /* LINK_H_ */
