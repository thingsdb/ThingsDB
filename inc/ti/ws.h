/*
 * ti/ws.h
 */
#ifndef TI_WS_H_
#define TI_WS_H_

#include <ex.h>
#include <ti/ws.t.h>
#include <ti/write.h>
#include <ti/req.t.h>
#include <uv.h>

int ti_ws_init(void);
int ti_ws_write(ti_ws_t * pss, ti_write_t * req);
char * ti_ws_name(const char * prefix, ti_ws_t * pss);
void ti_ws_destroy(void);

#endif /* TI_WS_H_ */
