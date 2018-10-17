/*
 * front.h
 */
#ifndef TI_FRONT_H_
#define TI_FRONT_H_

typedef enum
{
    TI_FRONT_PING,     // empty
    TI_FRONT_AUTH,     // [user, password]
    TI_FRONT_EVENT,    // {"target": [task,...]}
    TI_FRONT_GET,      // {"target": id}
} ti_front_req_e;


typedef struct ti_front_s  ti_front_t;

#include <ti/pkg.h>
struct ti_front_s
{
    ti_sock_t * sock;
};

ti_front_t * ti_front_create(void);
void ti_front_destroy(ti_front_t * front);
int ti_front_listen(ti_front_t * front);
int ti_front_write(ti_sock_t * sock, ti_pkg_t * pkg);

const char * ti_front_req_str(ti_front_req_e);

#endif /* TI_FRONT_H_ */


