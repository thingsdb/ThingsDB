#include <ti/tz.h>
#include <stdlib.h>

enum
{
    TOTAL_KEYWORDS = 433,
    MIN_WORD_LENGTH = 3,
    MAX_WORD_LENGTH = 30,
    MIN_HASH_VALUE = 22,
    MAX_HASH_VALUE = 1849
};


/* DO NOT EDIT THE LIST BELOW MANUALLY
 * Use `gen_tz_mapping.py` instead, and if changes
 */
static ti_tz_t tz__list[TOTAL_KEYWORDS] = {
    {.name="UTC",                                       .n=3,   .index=0    },
};


static ti_tz_t * tz__mapping[MAX_HASH_VALUE+1];

void ti_tz_init(void)
{
    for (size_t i = 0, n = TOTAL_KEYWORDS; i < n; ++i)
    {
        uint32_t key;
        ti_tz_t * tz = &tz__list[i];

        tz->n = strlen(tz->name);
        key = qbind__hash(tz->name, tz->n);

        assert (tz__mapping[key] == NULL);
        assert (key <= MAX_HASH_VALUE);

        tz__mapping[key] = tz;
    }
}
