/*
 * quota.c
 */
#include <stdint.h>
#include <ti/quota.h>



ti_quota_t * ti_quota_create(void)
{
    ti_quota_t * quota = malloc(sizeof(ti_quota_t));
    if (!quota)
        return NULL;

    quota->max_things = TI_QUOTA_NOT_SET;
    quota->max_props = TI_QUOTA_NOT_SET;
    quota->max_array_size = TI_QUOTA_NOT_SET;
    quota->max_raw_size = TI_QUOTA_NOT_SET;

    return quota;
}


ti_quota_enum_t ti_qouta_tp_from_strn(const char * str, size_t n, ex_t * e)
{
    int tp = strncmp(str, "things", n) == 0
        ? QUOTA_THINGS
        : strncmp(str, "properties", n) == 0
        ? QUOTA_PROPS
        : strncmp(str, "array_size", n) == 0
        ? QUOTA_ARRAY_SIZE
        : strncmp(str, "raw_size", n) == 0
        ? QUOTA_RAW_SIZE
        : 0;

    if (!tp)
    {
        ex_set(e, EX_INDEX_ERROR, "`quota_type` should be either "
            "`things`, `properties`, `array_size` or `raw_size`, "
            "got `%.*s`",
            (int) n, str);
    }
    return (ti_quota_enum_t) tp;
}
