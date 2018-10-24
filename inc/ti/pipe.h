/*
 * pipe.h
 */
#ifndef TI_PIPE_H_
#define TI_PIPE_H_

#include <uv.h>

char * ti_pipe_name(const char * prefix, uv_pipe_t * client);

#endif /* TI_PIPE_H_ */
