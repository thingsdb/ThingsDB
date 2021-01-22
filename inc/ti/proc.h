/*
 * ti/proc.h
 */
#ifndef TI_PROC_H_
#define TI_PROC_H_

typedef struct ti_proc_s ti_proc_t;

#include <ti/module.t.h>
#include <ti/proc.t.h>
#include <uv.h>

void ti_proc_init(ti_proc_t * proc, ti_module_t * module);
int ti_proc_load(ti_proc_t * proc);
int ti_proc_write_request(ti_proc_t * proc, uv_write_t * req, uv_buf_t * wrbuf);

#endif  /* TI_PROC_H_ */
