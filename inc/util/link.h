/*
 * link.h
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef LINK_H_
#define LINK_H_

typedef struct link_s  link_t;
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

/*
 * link_each is a rich looping
 */
#define link_each(link__, dt__, var__) \
        dt__ * var__, \
        ** l__ = (dt__ **) &link__->next_; \
        *l__ && \
        (var__ = (dt__ *) (*((struct link__s **) l__))->data_); \
        l__ = (dt__ **) &(*((struct link__s **) l__))->next_

#define link_pop_current(link__) \
        link__pop_current__(link__, (struct link__s **) l__)

#define link_insert_before(link__, data__) \
        link__insert_before__(link__, (struct link__s **) l__, data__)

#define link_insert_after(link__, data__) \
        link__insert_after__(link__, (struct link__s **) l__, data__)

#define link_replace_current(data__) \
        (*((struct link__s **) l__))->data_ = data__

/* private function */
void * link__pop_current__(link_t * link, struct link__s ** link_);
int link__insert_before__(link_t * link, struct link__s ** link_, void * data);
int link__insert_after__(link_t * link, struct link__s ** link_, void * data);
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


#endif /* LINK_H_ */
