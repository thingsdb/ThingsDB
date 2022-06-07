/*
 * ti/restore.h
 */
#ifndef TI_RESTORE_H_
#define TI_RESTORE_H_

#include <stddef.h>
#include <ex.h>
#include <ti/user.t.h>

char * ti_restore_task(const char * fn, size_t n);
int ti_restore_chk(const char * fn, size_t n, ex_t * e);
int ti_restore_unp(const char * restore_task, ex_t * e);
_Bool ti_restore_is_busy(void);
void ti_restore_finished(void);
int ti_restore_master(ti_user_t * user /* may be NULL */, _Bool restore_tasks);
int ti_restore_slave(void);

#endif  /* TI_RESTORE_H_ */
