/*
 * link.c
 */
#include <stdlib.h>
#include <util/link.h>

static struct link__s * link__new(void * data, struct link__s * next);


link_t * link_create(void)
{
    link_t * link = (link_t *) malloc(sizeof(link_t));
    if (!link)
        return NULL;
    link->next_ = NULL;
    link->n = 0;

    return link;
}

void link_destroy(link_t * link, link_destroy_cb cb)
{
    if (!link)
        return;
    struct link__s * cur = (struct link__s *) link;

    for (struct link__s * tmp; (tmp = cur->next_); cur = tmp)
    {
        if (cb && tmp) (*cb)(tmp->data_);
        free(cur);
    }
    free(cur);
}

/*
 * Inserts data at index inside the link. If index is >= link->n then the
 * data will be pushed at the end of the linked list.
 *
 * Return 0 when successful or -1 in case of an allocation error.
 */
int link_insert(link_t * link, size_t idx, void * data)
{
    struct link__s * cur, * tmp;
    for (cur = (struct link__s *) link; cur->next_ && idx--; cur = cur->next_);
    tmp = link__new(data, cur->next_);
    if (!tmp)
        return -1;
    link->n++;
    cur->next_ = tmp;

    return 0;
}

void * link_rm(link_t * link, void * data)
{
    struct link__s * cur, * prev = (struct link__s *) link;

    while ((cur = prev->next_))
    {
        if (cur->data_ == data)
        {
            prev->next_ = cur->next_;
            free(cur);
            return data;
        }
        prev = cur->next_;
    }
    return NULL;
}

void * link__pop__(link_t * link, struct link__s ** link_)
{
    struct link__s * cur = *link_;
    void * data = cur->data_;
    *link_ = cur->next_;
    free(cur);
    link->n--;

    return data;
}

/*
 * Removes the current item from the list and sets the iterator to the next
 * item in the list.
 */
void link_iter_remove(link_t * link, link_iter_t iter)
{
    struct link__s * cur = *iter;
    *iter = cur->next_;
    free(cur);
    link->n--;
}

int link_iter_insert(link_t * link, link_iter_t iter, void * data)
{
    struct link__s * tmp;

    if (*iter)
    {
        tmp = link__new((*iter)->data_, (*iter)->next_);
        if (!tmp) return -1;
        (*iter)->data_ = data;
        (*iter)->next_ = tmp;
    }
    else
    {
        tmp = link__new(data, NULL);
        if (!tmp) return -1;
        *iter = tmp;
    }

    link->n++;

    return 0;
}

static struct link__s * link__new(void * data, struct link__s * next)
{
    struct link__s * link = malloc(sizeof(struct link__s));
    if (!link)
        return NULL;

    link->data_ = data;
    link->next_ = next;

    return link;
}




