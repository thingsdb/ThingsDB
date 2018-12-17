#include "../test.h"
#include <util/big.h>


int big__mulii(void)
{
    test_start("big (mulii)");

    int64_t a = -9223372036854775807;
    // int64_t b = 9223372036854775807;
    // int64_t a = 5000;
    // int64_t b = 120;

    big_t * big1 = big_from_int64(a);
    big_t * big2 = big_mulbb(big1, big1);
    big_t * big3 = big_mulbi(big2, a);

    int64_t x = big_to_int64(big1);
    printf("x: %ld\n", x);

    {
        size_t sz = big_str16_msize(big1);
        char str[sz];
        int rc = big_to_str16n(big1, str, sz);
        printf("\nrc: %d, value: %s\n", rc, str);
        free(big1);
    }

    {
        size_t sz = big_str16_msize(big2);
        char str[sz];
        int rc = big_to_str16n(big2, str, sz);
        printf("\nrc: %d, value: %s\n", rc, str);
        free(big2);
    }

    {
        size_t sz = big_str16_msize(big3);
        char str[sz];
        int rc = big_to_str16n(big3, str, sz);
        printf("\nrc: %d, value: %s\n", rc, str);
        free(big3);
    }

    return test_end();
}

int main()
{
    return (
        big__mulii() ||
        0
    );
}
