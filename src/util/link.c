/*
 * link.c
 */
#include <stdlib.h>
#include <util/link.h>

static struct link__s * link__new(void * data, struct link__s * next)
{
    struct link__s * link = malloc(sizeof(struct link__s));
    if (!link)
        return NULL;

    link->data_ = data;
    link->next_ = next;

    return link;
}


link_t * link_create(void)
{
    link_t * link = malloc(sizeof(link_t));
    if (!link)
        return NULL;
    link->next_ = NULL;
    link->n = 0;

    return link;
}

void link_destroy(link_t * link, link_destroy_cb cb)
{
    link_clear(link, cb);
    free(link);
}

void link_clear(link_t * link, link_destroy_cb cb)
{
    struct link__s * node = link->next_;

    /* set to NULL in front, to prevent recursive calling */
    link->n = 0;
    link->next_ = NULL;

    while (node)
    {
        struct link__s * tmp = node;
        node = node->next_;
        if (cb)
            (*cb)(tmp->data_);
        free(tmp);
    }
}

void * link_rm(link_t * link, void * data)
{
    struct link__s * cur, * prev = (struct link__s *) link;

    while ((cur = prev->next_))
    {
        if (cur->data_ == data)
        {
            --link->n;
            prev->next_ = cur->next_;
            free(cur);
            return data;
        }
        prev = cur->next_;
    }
    return NULL;
}

/*
 * Removes the current item from the list and sets the iterator to the next
 * item in the list.
 */
int link_insert(link_t * link, void * data)
{
    struct link__s * tmp = link__new(data, link->next_);
    if (!tmp)
        return -1;
    link->next_ = tmp;
    ++link->n;
    return 0;
}






