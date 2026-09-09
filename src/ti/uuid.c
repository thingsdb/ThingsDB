/*
 * ti/uuid.c
 */
#include <ti/uuid.h>
#include <util/logger.h>

ti_uuid_t * ti_uuid_new(void)
{
    ti_uuid_t * uuid = malloc(sizeof(ti_uuid_t));
    if (!uuid)
        return NULL;
    uuid->ref = 1;
    uuid->tp = TI_VAL_UUID;
    return uuid;
}

/* requires `raw` to have at least length 36 (uuid_raw_t) */
void ti_uuid_to_raw(ti_uuid_t * uuid, char * raw)
{
    static const char hex_table[] = "0123456789abcdef";
    register uuid_t uuid_ = uuid->id;
    int p = 0;
    for (int i = 0; i < 16; i++)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
        {
            raw[p++] = '-';
        }

        uint8_t b = uuid_[i];
        raw[p++] = hex_table[b >> 4];   // High nibble
        raw[p++] = hex_table[b & 0x0F]; // Low nibble
    }
}

/* requires `str` to have at least length 37 (uuid_str_t) */
void ti_uuid_to_str(ti_uuid_t * uuid, char * str)
{
    ti_uuid_to_raw(uuid, str);
    str[36] = '\0';
}

ti_raw_t * ti_uuid_str(ti_uuid_t * uuid)
{
    ti_raw_t * raw = malloc(sizeof(ti_raw_t) + sizeof(uuid_raw_t));
    if (raw)
    {
        raw->n = sizeof(uuid_raw_t);
        raw->tp = TI_VAL_STR;
        ti_uuid_to_raw(uuid, raw->data);
    }
    return raw
}