/*
 * signals.c
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <uv.h>
#include <signal.h>
#include <rql/signals.h>
#include <util/logger.h>

static void rql__signals_handler(uv_signal_t * sig, int signum);

#define nsigs 6
static const int signms[nsigs] = {
        SIGHUP,
        SIGINT,
        SIGTERM,
        SIGSEGV,
        SIGABRT,
        SIGPIPE
};

static uv_signal_t signals[nsigs] = {0};

int rql_signals_init(rql_t * rql)
{
    /* bind signals to the event loop */
    for (int i = 0; i < nsigs; i++)
    {
        signals[i].data = rql;
        if (uv_signal_init(&rql->loop, &signals[i]) ||
            uv_signal_start(&signals[i], rql__signals_handler, signms[i]))
        {
            return -1;
        }
    }
    return 0;
}

static void rql__signals_handler(uv_signal_t * sig, int signum)
{
    rql_t * rql = (rql_t *) sig->data;

    if (signum == SIGPIPE)
    {
        log_warning("signal (%d) received, probably a connection was lost");
        return;
    }

    sig->flags |= RQL_FLAG_SIGNAL;

    if (signum == SIGINT || signum == SIGTERM || signum == SIGHUP)
    {
        log_warning("received stop signal (%s)", strsignal(signum));
    }

    uv_stop(&rql->loop);
}
