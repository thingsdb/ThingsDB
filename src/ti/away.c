/*
 * ti/away.c
 */
#include <assert.h>
#include <ti/away.h>
#include <ti.h>
#include <util/logger.h>

static ti_away_t * away;

static void away__destroy(uv_handle_t * UNUSED(handle));
static inline uint64_t away__calc_sleep(void);

int ti_away_create(void)
{
    away = malloc(sizeof(ti_nodes_t));
    if (!away)
        return -1;

    away->work = malloc(sizeof(uv_work_t));
    away->timer = malloc(sizeof(uv_timer_t));
    away->is_started = false;
    away->is_running = false;
    away->id = 0;

    if (!away->work || !away->timer)
    {
        away__destroy(NULL);
        return -1;
    }

    ti()->away = away;
    return 0;
}


int ti_away_start(void)
{
    assert (away->is_started == false);

    if (uv_timer_init(ti()->loop, away->timer))
        goto fail0;

    if (uv_timer_start(away->timer,
            away__timer_cb,
            away__calc_sleep(),
            away__calc_sleep()))
        goto fail1;

    away->is_started = true;
    return 0;

fail1:
    uv_close((uv_handle_t *) away->timer, away__destroy);
fail0:
    return -1;
}

void ti_away_stop(void)
{
    if (!away)
        return;

    if (!away->is_started)
        away__destroy(NULL);
    else
    {
        uv_timer_stop(away->timer);
        uv_close((uv_handle_t *) away->timer, away__destroy);
    }
}

static void away__destroy(uv_handle_t * UNUSED(handle))
{
    if (away)
    {
        free(away->work);
        free(away->timer);
        free(away);
    }
    away = ti()->away = NULL;
}

static void away__timer_cb(uv_timer_t * timer)
{
    if (away->is_running || ti_nodes_get_away())
        return;


    away->is_running = true;


}

static inline uint64_t away__calc_sleep(void)
{
    return ((away->id + ti()->node->id) % ti()->nodes->vec->n) * 5000 + 1000;
}
