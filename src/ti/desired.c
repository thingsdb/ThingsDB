/*
 * ti/desired.c
 */
#include <assert.h>
#include <ti.h>
#include <ti/desired.h>
#include <util/logger.h>


static ti_desired_t desired;

int ti_desired_create(void)
{
    desired.lookup = NULL;

    ti()->desired = &desired;
    return 0;
}

void ti_desired_destroy(void)
{
    if (!ti()->desired)
        return;

    ti_lookup_destroy(desired.lookup);

    ti()->desired = NULL;
}


int ti_desired_init(void)
{
    if (ti()->lookup->n == desired.n && ti()->lookup->r == desired.r)
    {
        assert (desired.lookup == NULL);

        log_debug("current state is equal to desired state");
        return 0;
    }

    if (desired.lookup)
    {
        log_debug("cleanup previous desired state");
        free(desired.lookup);
    }

    desired.lookup = ti_lookup_create(desired.n, desired.r);
    if (!desired.lookup)
    {
        log_critical(EX_ALLOC_S);
        return -1;
    }

    return 0;
}

int ti_desired_set(uint8_t n, uint8_t r)
{
    desired.n = n;
    desired.r = r;
    return -(
        ti_desired_init() ||
        ti_save()
    );
}
