/*
 * cryptx.c
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <util/cryptx.h>

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
 *  char[OWCRYPT_SZ] encrypted;
 *  owcrypt("my_password", "saltsalt$1", encrypted);
 *
 *  // Checking can be done like this:
 *  char[OWCRYPT_SZ] pw;
 *  owcrypt("pasword_to_check", encrypted, pw");
 *  if (strcmp(pw, encrypted) === 0) {
 *      // valid
 *  }
 *
 * Parameters:
 *  const char * password: must be a terminated string
 *  const char * salt: must be a string with at least a length OWCRYPT_SALT_SZ.
 *  char * encrypted: must be able to hold at least OWCRYPT_SZ chars.
 */
void cryptx(const char * password, const char * salt, char * encrypted)
{
    unsigned int i, j, c;
    unsigned long int k, t;
    unsigned const char * p, * w;

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
 *
 * Make sure random is initialized, for example using:
 *  srand(time(NULL));
 */
void cryptx_gen_salt(char * salt)
{
    int i;
    for (i = 0; i < CRYPTX_SALT_SZ - 2; i++)
    {
        salt[i] = VCHARS[rand() % 64];
    }
    salt[CRYPTX_SALT_SZ - 2] = '$';
    salt[CRYPTX_SALT_SZ - 1] = '1';
}

