/*
 * maint.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <rql/maint.h>

const int rql__maint_repeat = 5;

rql_maint_t * rql_maint_create(rql_t * rql)
{
    rql_maint_t * maint = (rql_maint_t *) malloc(sizeof(rql_maint_t));
    if (!maint) return NULL;
    maint->rql = rql;
    maint->timer.data = maint;
    maint->work.data = maint;
    maint->status = 0;
}


void rql_maint_destroy(rql_maint_t * maint)
{
    /* TODO: stop timer and or work ? */
    free(maint);
}

static void rql__maint_timer_cb(uv_timer_t * timer)
{
    rql_maint_t * maint = (rql_maint_t *) timer->data;

}

rql_maint_t * rql_maint_start(rql_maint_t * maint)
{
    uv_timer_init(&maint->rql->loop, &maint->timer);
    uv_timer_start(&maint->timer, rql__maint_timer_cb)
}
