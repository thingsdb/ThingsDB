/*
 * util/olist.h
 */
#include <assert.h>
#include <stdlib.h>
#include <util/olist.h>

static void olist__rm(olist_t * olist, olist__t ** olist_);
static olist__t * olist__new(uint64_t id, olist__t * next);

olist_t * olist_create(void)
{
    olist_t * olist = malloc(sizeof(olist_t));
    if (!olist)
        return NULL;

    olist->next_ = NULL;
    olist->n = 0;

    return olist;
}

void olist_destroy(olist_t * olist)
{
    if (!olist)
        return;
    olist__t * cur = (olist__t *) olist;

    for (olist__t * tmp; (tmp = cur->next_); cur = tmp)
        free(cur);

    free(cur);
}

/*
 * Return 0 if successful, or -1 in case of an allocation error.
 */
int olist_set(olist_t * olist, uint64_t id)
{
    assert(olist);
    olist__t * cur, * tmp;

    for (   cur = (olist__t *) olist;
            cur->next_ && cur->next_->id_ < id;
            cur = cur->next_);

    if (cur->next_ && cur->next_->id_ == id)
        return 0;

    tmp = olist__new(id, cur->next_);
    if (!tmp)
        return -1;

    olist->n++;
    cur->next_ = tmp;

    return 0;
}

_Bool olist_is_set(olist_t * olist, uint64_t id)
{
    olist__t * cur = (olist__t *) olist;
    while ((cur = cur->next_) && cur->id_ < id);

    return cur && cur->id_ == id;
}

void olist_rm(olist_t * olist, uint64_t id)
{
    olist__t * cur, * prev = (olist__t *) olist;
    while ((cur = prev->next_) && cur->id_ < id)
        prev = cur;

    if (cur && cur->id_ == id)
        olist__rm(olist, &prev->next_);
}

static void olist__rm(olist_t * olist, olist__t ** olist_)
{
    olist__t * cur = *olist_;
    *olist_ = cur->next_;

    free(cur);
    --olist->n;
}

static olist__t * olist__new(uint64_t id, olist__t * next)
{
    olist__t * olist = malloc(sizeof(olist__t));
    if (!olist)
        return NULL;

    olist->id_ = id;
    olist->next_ = next;

    return olist;
}
