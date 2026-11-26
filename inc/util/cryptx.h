/*
 * util/cryptx.h
 */
#ifndef CRYPTX_H_
#define CRYPTX_H_

#define CRYPTX_SZ 64
#define CRYPTX_SALT_SZ 10

#define CRYPTX_VERSION  0x01
#define CRYPTX_IV_LEN   12
#define CRYPTX_TAG_LEN  16
#define CRYPTX_KEY_LEN  32
#define CRYPTX_HDR_LEN  (1 + CRYPTX_IV_LEN + CRYPTX_TAG_LEN)
#define CRYPTX_ERROR_MEM -1
#define CRYPTX_ERROR_OTHER -2

#include <stddef.h>

void cryptx(const char *password, const char *salt, char *encrypted);
void cryptx_gen_salt(char *salt);

int cryptx_encrypt(
        const unsigned char  *in, size_t in_len,
        const unsigned char  *pass, size_t pass_len,
        unsigned char  **out, size_t *out_len);
int cryptx_decrypt(
        const unsigned char  *in, size_t in_len,
        const unsigned char  *pass, size_t pass_len,
        unsigned char  **out, size_t *out_len);

#endif /* CRYPTX_H_ */
