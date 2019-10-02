/*
 * ti/tcp.h
 */
#ifndef TI_TCP_H_
#define TI_TCP_H_

#include <uv.h>

const char * ti_tcp_ip_support_str(int ip_support);
char * ti_tcp_name(const char * prefix, uv_tcp_t * client);
int ti_tcp_addr(char * addr, uv_tcp_t * client);


#endif /* TI_TCP_H_ */
