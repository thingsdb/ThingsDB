#include <stdio.h>
#include <stdint.h>

void uuid_to_string(const uint8_t uuid[16], char *out) {
    static const char hex_table[] = "0123456789abcdef";

    // We schrijven direct naar de juiste posities in de 'out' buffer
    // Formaat: 8-4-4-4-12 (totaal 36 karakters + null)

    int p = 0;
    for (int i = 0; i < 16; i++) {
        // Voeg streepjes toe op de juiste posities
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            out[p++] = '-';
        }

        uint8_t b = uuid[i];
        out[p++] = hex_table[b >> 4];   // High nibble
        out[p++] = hex_table[b & 0x0F]; // Low nibble
    }
    out[36] = '\0'; // Null terminator
}

bool string_to_uuid(const char * in, uint8_t uuid[16]) {
    static const uint8_t hex_val[256] = {
        ['0']=0, ['1']=1, ['2']=2, ['3']=3, ['4']=4,
        ['5']=5, ['6']=6, ['7']=7, ['8']=8, ['9']=9,
        ['a']=10, ['b']=11, ['c']=12, ['d']=13, ['e']=14, ['f']=15,
        ['A']=10, ['B']=11, ['C']=12, ['D']=13, ['E']=14, ['F']=15
    };

    const char *p = in;
    for (int i = 0; i < 16; i++)
    {
        if (*p == '-') p++;

        uint8_t hi = hex_val[(unsigned char)*p++];
        uint8_t lo = hex_val[(unsigned char)*p++];

        uuid[i] = (hi << 4) | lo;
    }

    return true;
}