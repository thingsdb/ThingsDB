/*
 * ti/uuid.c
 */
#include <inttypes.h>
#include <ti/uuid.h>
#include <util/logger.h>
#include <util/util.h>


ti_uuid_t * ti_uuid_new(void)
{
    /* Returns a v7 uuid type; we might want to support other versions in
     * future */
    struct timeval tv;
    uint64_t ms;
    ti_uuid_t * uuid = malloc(sizeof(ti_uuid_t));
    if (!uuid)
        return NULL;
    uuid->ref = 1;
    uuid->tp = TI_VAL_UUID;

    gettimeofday(&tv, NULL);

    ms = (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
    uuid->id[0] = (ms >> 40) & 0xFF;
    uuid->id[1] = (ms >> 32) & 0xFF;
    uuid->id[2] = (ms >> 24) & 0xFF;
    uuid->id[3] = (ms >> 16) & 0xFF;
    uuid->id[4] = (ms >> 8) & 0xFF;
    uuid->id[5] = ms & 0xFF;
    util_get_random(&uuid->id[6], 10);
    uuid->id[6] = (uuid->id[6] & 0x0F) | 0x70;  /* 0111xxxx (Version: 7) */
    uuid->id[8] = (uuid->id[8] & 0x3F) | 0x80;  /* 10xxxxxx (Leach-Salz RFC) */

    return uuid;
}

ti_uuid_t * ti_uuid_from_bytes(const unsigned char * bytes)
{
    ti_uuid_t * uuid = malloc(sizeof(ti_uuid_t));
    if (!uuid)
        return NULL;
    uuid->ref = 1;
    uuid->tp = TI_VAL_UUID;
    memcpy(uuid->id, bytes, sizeof(uuid->id));

    return uuid;
}

static const uint8_t UUID__HEX_LUT[256] = {
    ['0']=0,  ['1']=1,  ['2']=2,  ['3']=3,  ['4']=4,
    ['5']=5,  ['6']=6,  ['7']=7,  ['8']=8,  ['9']=9,
    ['a']=10, ['b']=11, ['c']=12, ['d']=13, ['e']=14, ['f']=15,
    ['A']=10, ['B']=11, ['C']=12, ['D']=13, ['E']=14, ['F']=15,
    [0 ... '0'-1] = 0xFF,
    ['9'+1 ... 'A'-1] = 0xFF,
    ['F'+1 ... 'a'-1] = 0xFF,
    ['f'+1 ... 255] = 0xFF
};

static inline bool uuid__parse_hex_pair(const char * p, uint8_t * out)
{
    uint8_t hi = UUID__HEX_LUT[(unsigned char) p[0]];
    uint8_t lo = UUID__HEX_LUT[(unsigned char) p[1]];
    if ((hi | lo) & 0xF0) return false;
    *out = (hi << 4) | lo;
    return true;
}

ti_uuid_t * ti_uuid_from_str(const char * str, size_t n, ex_t * e)
{
    uuid_t uuid_;

    if (n == 36)
    {
        // Enforce exact hyphen positions upfront
        if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-')
        {
            ex_set(e, EX_VALUE_ERROR,
                   "invalid UUID format (hyphens missing or misplaced)");
            return NULL;
        }

        if (!uuid__parse_hex_pair(str + 0, (uint8_t *) &uuid_[0])  ||
            !uuid__parse_hex_pair(str + 2, (uint8_t *) &uuid_[1])  ||
            !uuid__parse_hex_pair(str + 4, (uint8_t *) &uuid_[2])  ||
            !uuid__parse_hex_pair(str + 6, (uint8_t *) &uuid_[3])  ||
            !uuid__parse_hex_pair(str + 9, (uint8_t *) &uuid_[4])  ||
            !uuid__parse_hex_pair(str + 11, (uint8_t *) &uuid_[5])  ||
            !uuid__parse_hex_pair(str + 14, (uint8_t *) &uuid_[6])  ||
            !uuid__parse_hex_pair(str + 16, (uint8_t *) &uuid_[7])  ||
            !uuid__parse_hex_pair(str + 19, (uint8_t *) &uuid_[8])  ||
            !uuid__parse_hex_pair(str + 21, (uint8_t *) &uuid_[9])  ||
            !uuid__parse_hex_pair(str + 24, (uint8_t *) &uuid_[10]) ||
            !uuid__parse_hex_pair(str + 26, (uint8_t *) &uuid_[11]) ||
            !uuid__parse_hex_pair(str + 28, (uint8_t *) &uuid_[12]) ||
            !uuid__parse_hex_pair(str + 30, (uint8_t *) &uuid_[13]) ||
            !uuid__parse_hex_pair(str + 32, (uint8_t *) &uuid_[14]) ||
            !uuid__parse_hex_pair(str + 34, (uint8_t *) &uuid_[15]))
        {
            ex_set(e, EX_VALUE_ERROR, "invalid hex character in UUID string");
            return NULL;
        }
    }
    else if (n == 32)
    {
        for (size_t i = 0; i < 16; i++)
        {
            if (!uuid__parse_hex_pair(str + (i * 2), (uint8_t *) &uuid_[i]))
            {
                ex_set(e, EX_VALUE_ERROR, "invalid hex character in UUID string");
                return NULL;
            }
        }
    }
    else
    {
        ex_set(e, EX_VALUE_ERROR, "invalid UUID string length");
        return NULL;
    }

    ti_uuid_t * uuid = malloc(sizeof(ti_uuid_t));
    if (!uuid)
    {
        ex_set_mem(e);
        return NULL;
    }

    uuid->ref = 1;
    uuid->tp = TI_VAL_UUID;
    memcpy(uuid->id, uuid_, sizeof(uuid_t));

    return uuid;
}

/* requires `raw` to have at least length 36 (uuid_raw_t) */
void ti_uuid_to_raw(uuid_t uuid, char * raw)
{
    static const char hex_table[] = "0123456789abcdef";
    int p = 0;
    for (int i = 0; i < 16; i++)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
        {
            raw[p++] = '-';
        }

        uint8_t b = uuid[i];
        raw[p++] = hex_table[b >> 4];   // High nibble
        raw[p++] = hex_table[b & 0x0F]; // Low nibble
    }
}

/* requires `str` to have at least length 37 (uuid_str_t) */
void ti_uuid_to_str(uuid_t uuid, char * str)
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
        ti_uuid_to_raw(uuid->id, (char *) raw->data);
    }
    return raw;
}