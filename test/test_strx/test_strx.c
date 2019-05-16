#include "../test.h"
#include <util/strx.h>


static int test_strx_to_double(void)
{
    test_start("strx (to_double)");
    _assert (strx_to_double("0.5") == 0.5);
    _assert (strx_to_double("0.55") == 0.5);
    _assert (strx_to_double("123.456") == 123.456);
    _assert (strx_to_double("123") == 123);
    _assert (strx_to_double("1234") == 1234);
    _assert (strx_to_double("123.") == 123);
    _assert (strx_to_double("+1234") == 1234);
    _assert (strx_to_double("-1234") == -1234);
    _assert (strx_to_double("-0.5") == -0.5);
    _assert (strx_to_double("-0.56") == -0.56);
    _assert (strx_to_double("+0.5") == 0.5);
    _assert (strx_to_double("+0.56") == 0.56);
    _assert (strx_to_double("-.5") == -0.5);
    _assert (strx_to_double("+.5") == 0.5);
    _assert (strx_to_double(".5") == 0.5);
    return test_end();
}

int main()
{
    return (
        test_strx_to_double() ||
        0
    );
}
