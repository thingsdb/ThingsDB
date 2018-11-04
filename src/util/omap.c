/*
 * util/omap.h
 */
#include <assert.h>
#include <stdlib.h>
#include <util/omap.h>

static void * omap__rm(omap_t * omap, struct omap__s ** omap_);
static struct omap__s * omap__new(void * data, struct omap__s * next);

omap_t * omap_create(void)
{
    omap_t * omap = (omap_t *) malloc(sizeof(omap_t));
    if (!omap)
        return NULL;

    omap->next_ = NULL;
    omap->n = 0;

    return omap;
}

void omap_destroy(omap_t * omap, omap_destroy_cb cb)
{
    if (!omap)
        return;
    struct omap__s * cur = (struct omap__s *) omap;

    for (struct omap__s * tmp; (tmp = cur->next_); cur = tmp)
    {
        if (cb && tmp)
            (*cb)(tmp->data_);
        free(cur);
    }
    free(cur);
}

/*
 * In case of a duplicate id the return value is OMAP_ERR_EXIST and data
 * will NOT be overwritten. On success the return value is OMAP_SUCCESS and
 * if a memory error has occurred the return value is OMAP_ERR_ALLOC.
 */
int omap_add(omap_t * omap, uint64_t id, void * data)
{
    assert (data);
    struct omap__s * cur, * tmp;

    for (   cur = (struct omap__s *) omap;
            cur->next_ && cur->next_->id < id;
            cur = cur->next_);

    if (cur->next_ && cur->next_->id == id)
        return OMAP_ERR_EXIST;

    tmp = omap__new(data, cur->next_);
    if (!tmp)
        return OMAP_ERR_ALLOC;

    omap->n++;
    cur->next_ = tmp;

    return OMAP_SUCCESS;
}

/*
 * In case of a duplicate id the return value is the previous value and data
 * will be overwritten. On success the return value is equal to void*data and
 * if a memory error has occurred the return value is NULL.
 */
void * omap_set(omap_t * omap, uint64_t id, void * data)
{
    assert (data);
    struct omap__s * cur, * tmp;

    for (   cur = (struct omap__s *) omap;
            cur->next_ && cur->next_->id < id;
            cur = cur->next_);

    if (cur->next_ && cur->next_->id == id)
    {
        void * prev = cur->next_->data_;
        cur->next_->data_ = data;
        return prev;
    }

    tmp = omap__new(data, cur->next_);
    if (!tmp)
        return NULL;

    omap->n++;
    cur->next_ = tmp;

    return data;
}

void * omap_get(omap_t * omap, uint64_t id)
{
    struct omap__s * cur = (struct omap__s *) omap;
    while ((cur = cur->next_) && cur->id <= id)
        if (cur->id == id)
            return cur->data_;
    return NULL;
}

void * omap_rm(omap_t * omap, uint64_t id)
{
    struct omap__s * cur = (struct omap__s *) omap;
    while ((cur = cur->next_) && cur->id <= id)
        if (cur->id == id)
            return omap__rm(omap, &cur);
    return NULL;
}

static void * omap__rm(omap_t * omap, struct omap__s ** omap_)
{
    struct omap__s * cur = *omap_;
    void * data = cur->data_;
    *omap_ = cur->next_;
    free(cur);
    omap->n--;

    return data;
}

static struct omap__s * omap__new(void * data, struct omap__s * next)
{
    struct omap__s * omap = malloc(sizeof(struct omap__s));
    if (!omap)
        return NULL;

    omap->data_ = data;
    omap->next_ = next;

    return omap;
}
