/*
 * tcp.h
 */
#ifndef TI_TCP_H_
#define TI_TCP_H_

#include <uv.h>

const char * ti_tcp_ip_support_str(uint8_t ip_support);
char * ti_tcp_name(uv_tcp_t * client);

#endif /* TI_TCP_H_ */
