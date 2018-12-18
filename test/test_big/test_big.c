#include "../test.h"
#include <limits.h>
#include <util/big.h>

#define big__sz 4096
static char str[big__sz];


int big__from_str2n(void)
{
    test_start("big (from_str2n)");

    big_t * big;

    big = big_from_str2n("00101110", 8);

    _assert (strcmp(str, "3fffffffffffffff0000000000000001") == 0);


    free(big);

    return test_end();
}

int big__fits_int64(void)
{
    test_start("big (fits_int64)");

    big_t * big = big_from_int64(LLONG_MIN);
    _assert (big_fits_int64(big));

    big->negative_ = 0;
    _assert (!big_fits_int64(big));
    free(big);

    return test_end();
}

int big__mulbd(void)
{
    test_start("big (mulbd)");
    double d, b;
    big_t * a, * big;

    a = big_from_int64(0x7fffffffffffffff);
    big = big_mulbb(a, a);

    b = 5.5e+40;
    d = big_mulbd(big, b);
    sprintf(str, "%f", d);
    _assert (strncmp(str, "46788825451629", 14) == 0);

    free(a);
    free(big);

    return test_end();
}

int big__mulbi(void)
{
    test_start("big (mulbi)");
    int64_t b;
    big_t * a, * big;

    a = big_from_int64(0x7fffffffffffffff);
    b = 0x7fffffffffffffff;

    big = big_mulbi(a, b);
    _assert (!big_is_null(big));
    _assert (big_is_positive(big));
    _assert (big_to_str16n(big, str, big__sz));
    _assert (strcmp(str, "3fffffffffffffff0000000000000001") == 0);
    free(big);
    free(a);

    a = big_from_int64(-0x7fffffffffffffff);
    big = big_mulbi(a, b);
    _assert (big_is_negative(big));
    _assert (big_to_str16n(big, str, big__sz));
    _assert (strcmp(str, "3fffffffffffffff0000000000000001") == 0);
    free(big);
    free(a);

    a = big_from_int64(0);
    big = big_mulbi(a, b);
    _assert (big_is_null(big));
    _assert (big_is_positive(big));
    _assert (big_to_str16n(big, str, big__sz));
    _assert (strcmp(str, "0") == 0);
    free(big);
    free(a);

    return test_end();
}

int big__mulii(void)
{
    test_start("big (mulii)");
    big_t * big;
    int64_t a, b;

    a = b = LLONG_MAX;
    big = big_mulii(a, b);
    _assert (!big_is_null(big));
    _assert (big_is_positive(big));
    _assert (big_to_str16n(big, str, big__sz));
    _assert (strcmp(str, "3fffffffffffffff0000000000000001") == 0);
    free(big);

    a = LLONG_MIN;
    big = big_mulii(a, b);
    _assert (big_is_negative(big));
    _assert (big_to_str16n(big, str, big__sz));
    _assert (strcmp(str, "3fffffffffffffff8000000000000000") == 0);
    free(big);

    a = 0;
    big = big_mulii(a, b);
    _assert (big_is_null(big));
    _assert (big_is_positive(big));
    _assert (big_to_str16n(big, str, big__sz));
    _assert (strcmp(str, "0") == 0);
    free(big);

    return test_end();
}

int main()
{
    return (
        big__mulii() ||
        big__mulbi() ||
        big__mulbd() ||
        big__fits_int64() ||
        0
    );
}
