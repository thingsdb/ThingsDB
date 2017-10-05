#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#define TEST_OK 0
#define TEST_FAILED -1
#define TEST_MSG_OK     "....OK"
#define TEST_MSG_FAILED "FAILED"

static struct timeval start;
static struct timeval end;

const char * padding =
        ".............................."
        "..............................";

static void test_start(char * test_name)
{
    int padlen = 60 - strlen(test_name);
    printf("Testing %s%*.*s", test_name, padlen, padlen, padding);
    gettimeofday(&start, 0);
}

static int test_end(int status)
{
    gettimeofday(&end, 0);
    float t = (end.tv_sec - start.tv_sec) * 1000.0f +
            (end.tv_usec - start.tv_usec) / 1000.0f;

    printf("%s (%.3f ms)\n",
            (status == TEST_OK) ? TEST_MSG_OK : TEST_MSG_FAILED,
                    t);

    return status;
}
