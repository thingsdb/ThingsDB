#include "../test.h"
#include <util/strx.h>


static int test_strx_to_doublen(void)
{
    test_start("strx (to_doublen)");
    _assert (strx_to_doublen("0.5", 3) == 0.5);
    _assert (strx_to_doublen("0.55", 3) == 0.5);
    _assert (strx_to_doublen("123.456", 7) == 123.456);
    _assert (strx_to_doublen("123", 3) == 123);
    _assert (strx_to_doublen("1234", 3) == 123);
    _assert (strx_to_doublen("123.", 4) == 123);
    _assert (strx_to_doublen("123.", 3) == 123);
    _assert (strx_to_doublen("+1234", 4) == 123);
    _assert (strx_to_doublen("-1234", 4) == -123);
    _assert (strx_to_doublen("123456.", 3) == 123);
    _assert (strx_to_doublen("-0.5", 4) == -0.5);
    _assert (strx_to_doublen("-0.56", 4) == -0.5);
    _assert (strx_to_doublen("+0.5", 4) == 0.5);
    _assert (strx_to_doublen("+0.56", 4) == 0.5);
    _assert (strx_to_doublen("-.5", 3) == -0.5);
    _assert (strx_to_doublen("+.55", 3) == 0.5);
    _assert (strx_to_doublen(".55", 2) == 0.5);
    _assert (strx_to_doublen("-.55", 3) == -0.5);
    return test_end();
}

int main()
{
    return (
        test_strx_to_doublen() ||
        0
    );
}
