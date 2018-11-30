/*
 * ti/skipped.h
 */
#ifndef TI_SKIPPED_H_
#define TI_SKIPPED_H_


typedef struct ti_skipped_s ti_skipped_t;

ti_skipped_t * ti_skipped_create(void);
void ti_skipped_destroy(ti_skipped_t * skipped);
void ti_skipped_clear(ti_skipped_t * skipped);
int ti_skipped_to_disk(ti_skipped_t * skipped);


struct ti_skipped_s
{
    vec_t * vec;        /* ti_event_t */
};

#endif  /* TI_SKIPPED_H_ */
