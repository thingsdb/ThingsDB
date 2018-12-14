#include "../test.h"
#include <util/big.h>


int big__mulii(void)
{
    test_start("big (mulii)");

    int64_t a = 0xffff;
    int64_t b = 0xfffff;

    big_t * big = big_mulii(a, b);

    size_t sz = big_str16_msize(big);

    char str[sz];

    int rc = big_to_str16n(big, str, sz);

    printf("\nrc: %d, value: %s\n", rc, str);

    free(big);


    return test_end();
}

int main()
{
    return (
        big__mulii() ||
        0
    );
}
