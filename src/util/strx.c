/*
 * util/strx.h
 */
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <lib/simdutf8check.h>

#define STRX__MAX_CONV_SZ 50
static char strx__buf[STRX__MAX_CONV_SZ];

void strx_lower_case(char * str)
{
   for (; *str; str++)
       *str = tolower(*str);
}

void strx_upper_case(char * str)
{
   for (; *str; str++)
       *str = toupper(*str);
}

void strx_replace_char(char * str, char find, char replace)
{
    for (; *str; str++)
        if (*str == find)
            *str = replace;
}

/*
 * Replace all occurrences of 'o' with 'r' in 'str'. We restrict the new size
 * to 'n'.
 *
 * Returns 0 if successful or -1 if the replaced string does not fit. In this
 * case the original string is untouched. The new string is terminated.
 */
int strx_replace_str(char * str, char * o, char * r, size_t n)
{
    char buffer[n];
    char * pos, * s;
    size_t l, size = 0, olen = strlen(o), rlen = strlen(r);

    for (s = str; (pos = strstr(s, o)) != NULL; s = pos + olen)
    {
        l = pos - s;

        if (size + l + rlen >= n)
        {
            return -1;
        }

        memcpy(buffer + size, s, l);
        size += l;

        memcpy(buffer + size, r, rlen);
        size += rlen;

    }

    if (s != str)
    {
        memcpy(str, buffer, size);
        str[size] = '\0';
    }

    return 0;
}

/*
 * Split and then join a given string.
 *
 * For example:
 *      string: "  this  is a   test  "
 *      split: ' ' and join with '_'
 *      result: "this_is_a_test"
 */
void strx_split_join(char * pt, char split_chr, char join_chr)
{
    int join = -1;
    char * dest = pt;

    for (; *pt != '\0'; pt++)
    {
        if (*pt != split_chr)
        {
            if (join > 0)
            {
                *dest = join_chr;
                dest++;
            }
            join = 0;
            *dest = *pt;
            dest++;
        }
        else if (!join)
        {
            join = 1;
        }
    }

    *dest = '\0';
}

void strx_trim(char ** str, char chr)
{
    /*
     * trim: when chr is 0 we will trim whitespace,
     * otherwise only the given char.
     */
    char * end;

    // trim leading chars
    while ((chr && **str == chr) || (!chr && isspace(**str)))
    {
        (*str)++;
    }

    // check all chars?
    if(**str == 0)
    {
        return;
    }

    // trim trailing chars
    end = *str + strlen(*str) - 1;
    while (end > *str && ((chr && *end == chr) || (!chr && isspace(*end))))
    {
        end--;
    }

    // write new null terminator
    *(end + 1) = 0;
}

/*
 * returns true or false
 */
_Bool strx_is_empty(const char * str)
{
    for (; *str; str++)
        if (!isspace(*str))
            return false;
    return true;
}

_Bool strx_is_int(const char * str)
{
   // Handle signed numbers.
   if (*str == '-' || *str == '+')
       ++str;

   // Handle empty string or only signed.
   if (!*str)
       return false;

   // Check for non-digit chars in the rest of the string.
   for (; *str; ++str)
       if (!isdigit(*str))
           return false;

   return true;
}

_Bool strx_is_float(const char * str)
{
   size_t dots = 0;

   // Handle signed float numbers.
   if (*str == '-' || *str == '+')
       ++str;

   // Handle empty string or only signed.
   if (!*str)
       return false;

   // Check for non-digit chars in the rest of the string.
   for (; *str; ++str)
       if (*str == '.')
           ++dots;
       else if (!isdigit(*str))
           return false;

   return dots == 1;
}

_Bool strx_is_graph(const char * str)
{
    for (; *str; str++)
        if (!isgraph(*str))
            return false;
    return true;
}

_Bool strx_is_graphn(const char * str, size_t n)
{
    for (; n--; str++)
        if (!isgraph(*str))
            return false;
    return true;
}

_Bool strx_is_printable(const char * str)
{
    for (; *str; str++)
        if (!isprint(*str))
            return false;
    return true;
}

_Bool strx_is_printablen(const char * str, size_t n)
{
    for (; n--; str++)
        if (!isprint(*str))
            return false;
    return true;
}

_Bool strx_is_asciin(const char * str, size_t n)
{
    for (; n--; str++)
        if (*str < 0)
            return false;
    return true;
}

_Bool strx_is_utf8n(const char * str, size_t n)
{
    return validate_utf8_fast(str, n);
}

/*
 * Requires a match with regular expression:
 *
 *      [-+]?((inf|nan)([^0-9A-Za-z_]|$)|[0-9]*\.[0-9]+(e[+-][0-9]+)?)
 */
double strx_to_double(const char * str)
{
    double d;
    double negative = 0;

    switch (*str)
    {
    case '-':
        negative = -1.0;
        ++str;
        break;
    case '+':
        ++str;
        break;
    }

    if (*str == 'i')
        return negative ? negative * INFINITY : INFINITY;

    if (*str == 'n')
        return NAN;

    if (errno == ERANGE)
        errno = 0;

    d = strtod(str, NULL);

    if (errno == ERANGE)
    {
        assert (d == HUGE_VAL || d == -HUGE_VAL);

        d = d == HUGE_VAL ? INFINITY : -INFINITY;
        errno = 0;
    }

    return negative ? negative * d : d;
}

/*
 * Requires a match with regular expression:
 *
 *      [-+]?((0b[01]+)|(0o[0-8]+)|(0x[0-9a-fA-F]+)|([0-9]+))
 *
 * This function may set errno to ERANGE
 */
int64_t strx_to_int64(const char * str)
{
    int64_t i;
    int negative = 0;
    int base = 10;

    switch (*str)
    {
    case '-':
        negative = -1;
        ++str;
        break;
    case '+':
        ++str;
        break;
    }

    if (*str == '0')
    {
        ++str;
        switch(*str)
        {
        case 'b':
            base = 2;
            ++str;
            break;
        case 'o':
            base = 8;
            ++str;
            break;
        case 'x':
            base = 16;
            ++str;
            break;
        case '\0':
            return 0;
        }
    }

    if (errno == ERANGE)
        errno = 0;

    i = strtoll(str, NULL, base);

    return negative ? negative * i : i;
}

/* not thread safe, the returned string might not be NULL terminated when the
 * output exceeds STRX__MAX_CONV_SZ. If this is the case,
 * then size `n` is set to 0. */
const char * strx_from_double(const double d, size_t * n)
{
    int r = snprintf(strx__buf, STRX__MAX_CONV_SZ, "%g", d);
    *n = (r < 0 || r >= STRX__MAX_CONV_SZ) ? 0 : (size_t) r;
    return strx__buf;
}

/* not thread safe, the returned string might not be NULL terminated when the
 * output exceeds STRX__MAX_CONV_SZ. If this is the case,
 * then size `n` is set to 0. */
const char * strx_from_int64(const int64_t i, size_t * n)
{
    int r = snprintf(strx__buf, STRX__MAX_CONV_SZ, "%"PRId64, i);
    *n = (r < 0 || r >= STRX__MAX_CONV_SZ) ? 0 : (size_t) r;
    return strx__buf;
}

char * strx_cat(const char * s1, const char * s2)
{
    size_t n1 = strlen(s1);
    size_t n2 = strlen(s2);

    char * s = (char *) malloc(n1 + n2 + 1);
    if (!s)
        return NULL;

    memcpy(s, s1, n1);
    memcpy(s + n1, s2, n2 + 1);

    return s;
}
