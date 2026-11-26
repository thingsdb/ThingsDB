/*
 * cryptx.c
 */
#include <assert.h>
#include <util/cryptx.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define CRYPTSET(x) t = encrypted[x] + k; encrypted[x] = VCHARS[t % 64]

#define VCHARS "./0123456789" \
    "abcdefghijklmnopqrstuvwxyz"  \
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

static const int P[109] = {
        281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367,
        373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449,
        457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547,
        557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631,
        641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727,
        733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823,
        827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919,
        929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997 };

static const unsigned int END = CRYPTX_SZ - 1;

/*
 * Encrypts a password using a salt.
 *
 * Usage:
 *
 *  char salt[CRYPTX_SALT_SZ];
 *  char encrypted[CRYPTX_SZ];
 *
 *  // Generate a random salt
 *  cryptx_gen_salt(salt);
 *
 *  // Create encryption
 *  cryptx("my_password", salt, encrypted);
 *
 *  // Checking can be done like this:
 *  char validate[CRYPTX_SZ];
 *  cryptx("pasword_to_check", encrypted, validate);
 *  if (strcmp(validate, encrypted) == 0) {
 *      // valid
 *  }
 *
 * Parameters:
 *  const char *password: must be a terminated string
 *  const char *salt: must be a string with at least a length OWCRYPT_SALT_SZ.
 *  char *encrypted: must be able to hold at least CRYPTX_SZ chars and will
 *                    be a null terminated string. (CRYPTX_SZ - 1 is the actual
 *                    length)
 */
void cryptx(const char *password, const char *salt, char *encrypted)
{
    unsigned int i, j, c;
    unsigned long int k, t;
    unsigned const char *p, *w;

    memset(encrypted, 0, CRYPTX_SZ);

    for (i = 0; i < CRYPTX_SALT_SZ; i++)
    {
        encrypted[i] = salt[i];
    }

    for (   w = (unsigned char *) password, i = CRYPTX_SALT_SZ;
            i < END;
            i++, w++)
    {
        if (!*w)
        {
            w = (unsigned char *) password;
        }

        for (k = 0, p = (unsigned char *) password; *p; p++)
        {
            for (c = 0; c < CRYPTX_SALT_SZ; c++)
            {
                k += P[(salt[c] + *p + *w + i + k) % 109];
            }
        }
        CRYPTSET(i);

        for (j = k % 3 + CRYPTX_SALT_SZ, c = k % 5 + 1; j < END; j += c)
        {
            CRYPTSET(j);
        }
    }
}

/*
 * Generate a random salt.
 */
void cryptx_gen_salt(char *salt)
{
    int i;
    for (i = 0; i < CRYPTX_SALT_SZ - 2; i++)
    {
        salt[i] = VCHARS[rand() % 64];
    }
    salt[CRYPTX_SALT_SZ - 2] = '$';
    salt[CRYPTX_SALT_SZ - 1] = '1';
}

static int cryptx__derive_key(
        const void *pass,
        size_t pass_len,
        unsigned char out_key[CRYPTX_KEY_LEN])
{
    unsigned int out_len = 0;
    if (EVP_Digest(pass, pass_len, out_key, &out_len, EVP_sha256(), NULL) != 1)
        return -1;
    return out_len == CRYPTX_KEY_LEN ? 0 : -1;
}

/* Returns 0 and sets out binary buffer (caller frees *out) or -1 on error */
int cryptx_encrypt(
        const unsigned char *in, size_t in_len,
        const unsigned char *pass, size_t pass_len,
        unsigned char **out, size_t *out_len)
{
    unsigned char key[CRYPTX_KEY_LEN];
    unsigned char *buf = malloc(CRYPTX_HDR_LEN + in_len);
    if (!buf)
        return -1;

    buf[0] = CRYPTX_VERSION;
    unsigned char *iv  = buf + 1;
    unsigned char *tag = buf + 1 + CRYPTX_IV_LEN;
    unsigned char *ct  = buf + CRYPTX_HDR_LEN;

    if (RAND_bytes(iv, CRYPTX_IV_LEN) != 1)
        goto fail;

    cryptx__derive_key(pass, pass_len, key);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len = 0, ct_len = 0, ok = 0;

    if (ctx &&
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, CRYPTX_IV_LEN, NULL) == 1 &&
        EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) == 1 &&
        EVP_EncryptUpdate(ctx, ct, &len, in, (int) in_len) == 1)
    {
        ct_len = len;
        if (EVP_EncryptFinal_ex(ctx, ct + len, &len) == 1)
        {
            ct_len += len;
            if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, CRYPTX_TAG_LEN, tag) == 1)
                ok = 1;
        }
    }

    OPENSSL_cleanse(key, sizeof(key));
    EVP_CIPHER_CTX_free(ctx);

    if (!ok)
        goto fail;

    *out = buf;
    *out_len = CRYPTX_HDR_LEN + (size_t) ct_len;
    return 0;

fail:
    free(buf);
    return -1;
}

/* Decrypts raw binary buffer (after Base64 decode); returns -1 if tampered
 * or wrong key */
int cryptx_decrypt(
        const unsigned char *in, size_t in_len,
        const unsigned char *pass, size_t pass_len,
        unsigned char **out, size_t *out_len)
{
    if (in_len < CRYPTX_HDR_LEN || in[0] != CRYPTX_VERSION)
        return CRYPTX_ERROR_OTHER;

    const unsigned char *iv  = in + 1;
    const unsigned char *tag = in + 1 + CRYPTX_IV_LEN;
    const unsigned char *ct  = in + CRYPTX_HDR_LEN;
    size_t ct_len = in_len - CRYPTX_HDR_LEN;

    unsigned char key[CRYPTX_KEY_LEN];
    unsigned char *pt = malloc(ct_len + 1);
    if (!pt)
        return CRYPTX_ERROR_MEM;

    cryptx__derive_key(pass, pass_len, key);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len = 0, pt_len = 0, ok = 0;

    if (ctx &&
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, CRYPTX_IV_LEN, NULL) == 1 &&
        EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv) == 1 &&
        EVP_DecryptUpdate(ctx, pt, &len, ct, (int) ct_len) == 1)
    {
        pt_len = len;
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, CRYPTX_TAG_LEN, (void *) tag) == 1 &&
            EVP_DecryptFinal_ex(ctx, pt + len, &len) == 1)
        {
            pt_len += len;
            pt[pt_len] = '\0';
            ok = 1;
        }
    }

    OPENSSL_cleanse(key, sizeof(key));
    EVP_CIPHER_CTX_free(ctx);

    if (!ok)
    {
        free(pt);
        return CRYPTX_ERROR_OTHER;
    }

    *out = pt;
    *out_len = (size_t) pt_len;
    return 0;
}

