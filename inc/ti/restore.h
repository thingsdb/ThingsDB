/*
 * ti/restore.h
 */
#ifndef TI_RESTORE_H_
#define TI_RESTORE_H_

#include <stddef.h>
#include <ex.h>

char * ti_restore_job(const char * fn, size_t n);
int ti_restore_chk(const char * fn, size_t n, ex_t * e);
int ti_restore_unp(const char * restore_job, ex_t * e);
int ti_restore_master(void);
int ti_restore_slave(void);

#endif  /* TI_RESTORE_H_ */
