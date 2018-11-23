/*
 * user.h
 */
#ifndef TI_USER_H_
#define TI_USER_H_

typedef struct ti_user_s  ti_user_t;

#include <stdint.h>
#include <ti/raw.h>
#include <ti/ex.h>

extern const char * ti_user_def_name;
extern const char * ti_user_def_pass;
extern const unsigned int ti_min_name;  // length including terminator
extern const unsigned int ti_max_name;  // length including terminator
extern const unsigned int ti_min_pass;  // length including terminator
extern const unsigned int ti_max_pass;  // length including terminator

ti_user_t * ti_user_create(
        uint64_t id,
        const char * name,
        size_t n,
        const char * encrpass);
void ti_user_drop(ti_user_t * user);
_Bool ti_user_name_check(const char * name, size_t n, ex_t * e);
_Bool ti_user_pass_check(const char * passstr, ex_t * e);
int ti_user_rename(ti_user_t * user, ti_raw_t * name);
int ti_user_set_pass(ti_user_t * user, const char * pass);

struct ti_user_s
{
    uint32_t ref;
    uint64_t id;
    ti_raw_t * name;
    char * encpass;
    unsigned char * qpdata;       /* qp_map with properties */
};

#endif /* TI_USER_H_ */
