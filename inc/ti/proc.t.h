/*
 * ti/proc.t.h
 */
#ifndef TI_PROC_T_H_
#define TI_PROC_T_H_

typedef struct ti_proc_s ti_proc_t;

#include <ti/module.t.h>
#include <util/buf.h>
#include <uv.h>

struct ti_proc_s
{
    uv_process_t process;
    uv_process_options_t options;
    uv_stdio_container_t child_stdio[3];
    uv_pipe_t child_stdin;
    uv_pipe_t child_stdout;
    ti_module_t * module;
    buf_t buf;
};

#endif  /* TI_PROC_T_H_ */
