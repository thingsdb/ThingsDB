/*
 * ti/datetime.c
 */
#define _GNU_SOURCE
#include <ti/datetime.h>
#include <ti/val.t.h>
#include <ti/raw.inline.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/*
 * Arguments `ts` is in seconds, `offset` in minutes.
 */
ti_datetime_t * ti_datetime_from_i64(int64_t ts, int16_t offset)
{
    ti_datetime_t * dt = malloc(sizeof(ti_datetime_t));
    if (!dt)
        return NULL;
    dt->ref = 1;
    dt->tp = TI_VAL_DATETIME;
    dt->ts = (time_t) ts;
    dt->offset = offset;
    return dt;
}

ti_datetime_t * ti_datetime_copy(ti_datetime_t * dt)
{
    ti_datetime_t * dtnew = malloc(sizeof(ti_datetime_t));
    memcpy(dtnew, dt, sizeof(ti_datetime_t));
    return dtnew;
}

static long int datetime__offset_in_sec(const char * str, ex_t * e)
{
    int sign;
    long int hours, minutes;

    switch (*str)
    {
    case '-':
        sign = -1;
        break;
    case '+':
        sign = 1;
        break;
    default:
        goto invalid;
    }

    if (!isdigit(*(++str)))
        goto invalid;

    hours = (*str - '0') * 10;

    if (!isdigit(*(++str)))
        goto invalid;

    hours += (*str - '0');

    if (hours > 12)
        goto invalid;

    if (isdigit(*(++str)))
    {
        /* read minutes */
        minutes = (*str - '0') * 10;

        if (!isdigit(*(++str)))
            goto invalid;

        minutes += (*str - '0');

        if (minutes > 59 || (minutes && hours == 12))
            goto invalid;

        ++str;
    }
    else
        minutes = 0;

    if (*str)  /* must be the null terminator character */
        goto invalid;

    return (hours * 3600 + minutes * 60) * sign;

invalid:
    ex_set(e, EX_VALUE_ERROR,
            "invalid offset; "
            "expecting format `+/-hh[mm]`, for example +0100 or -05");
    return 0;
}

long int ti_datetime_offset_in_sec(ti_raw_t * str, ex_t * e)
{
    const size_t buf_sz = 6;
    char buf[buf_sz];
    if (str->n >= buf_sz)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid offset; "
                "expecting format `+/-hh[mm]`, for example +0100 or -05");
        return 0;
    }

    memcpy(buf, str->data, str->n);
    buf[str->n] = '\0';

    return datetime__offset_in_sec(buf, e);
}

static ti_datetime_t * datetime__from_str(
        ti_raw_t * str,
        const char * fmt,
        _Bool with_offset,
        ex_t * e)
{
    ti_datetime_t * dt;
    time_t ts;
    long int offset = 0;
    struct tm tm;
    const size_t buf_sz = 80;
    char buf[buf_sz];
    char * c;

    if (str->n >= buf_sz)
    {
        ex_set(e, EX_VALUE_ERROR,
                "invalid datetime string (too large)");
        return NULL;
    }

    memcpy(buf, str->data, str->n);
    buf[str->n] = '\0';

    memset(&tm, 0, sizeof(struct tm));
    tm.tm_mday = 1;  /* must be set to 1 */

    c = strptime(buf, fmt, &tm);
    if (c == NULL || *c)
        goto invalid;

    ts = mktime(&tm);

    if (with_offset)
    {
        size_t n = str->n;
        while (n--)
            if (buf[n] == '+' || buf[n] == '-')
                break;

        if (n > str->n)
            goto invalid;

        offset = datetime__offset_in_sec(buf + n, e);
        if (e->nr)
            return NULL;

        if ((offset < 0 && ts > LLONG_MAX + offset) ||
            (offset > 0 && ts < LLONG_MIN + offset))
        {
            ex_set(e, EX_OVERFLOW, "datetime overflow");
            return NULL;
        }

        ts -= offset;
        offset /= 60;
    }

    dt = ti_datetime_from_i64(ts, (int16_t) offset /* offset is checked */);
    if (!dt)
    {
        ex_set_mem(e);
        return NULL;
    }
    return dt;

invalid:
    ex_set(e, EX_VALUE_ERROR,
            "invalid datetime string (does not match format string `%s`)", fmt);
    return NULL;
}

static inline _Bool datetime__with_t(ti_raw_t * str)
{
    return str->data[10] == 'T';
}

ti_datetime_t * ti_datetime_from_str(ti_raw_t * str, ex_t * e)
{
    switch(str->n)
    {
    case 4:  /* YYYY */
        return datetime__from_str(str, "%Y", 0 /* no offset */, e);
    case 7:  /* YYYY-MM */
        return datetime__from_str(str, "%Y-%m", 0, e);
    case 10: /* YYYY-MM-DD */
        return datetime__from_str(str, "%Y-%m-%d", 0, e);
    case 13: /* YYYY-MM-DDTHH */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H", 0, e)
                : datetime__from_str(str, "%Y-%m-%d %H", 0, e);
    case 16: /* YYYY-MM-DDTHH:MM */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M", 0, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M", 0, e);
    case 19: /* YYYY-MM-DDTHH:MM:SS */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M:%S", 0, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M:%S", 0, e);
    case 20: /* YYYY-MM-DDTHH:MM:SSZ */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M:%SZ", 0, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M:%SZ", 0, e);
    case 22: /* YYYY-MM-DDTHH:MM:SS+hh */
    case 24: /* YYYY-MM-DDTHH:MM:SS+hhmm */
        return datetime__with_t(str)
                ? datetime__from_str(str, "%Y-%m-%dT%H:%M:%S%z", 1, e)
                : datetime__from_str(str, "%Y-%m-%d %H:%M:%S%z", 1, e);
    }

    ex_set(e, EX_VALUE_ERROR,
            "invalid datetime string");
    return NULL;
}

#define DATETIME__BUF_SZ 32
static char datetime__buf[DATETIME__BUF_SZ];
static const char * datetime__fmt_zone = "%Y-%m-%dT%H:%M:%S%z";
static const char * datetime__fmp_utc = "%Y-%m-%dT%H:%M:%SZ";


ti_datetime_t * ti_datetime_from_fmt(ti_raw_t * str, ti_raw_t * fmt, ex_t * e)
{
    _Bool with_offset;

    if (fmt->n < 2 || fmt->n >= DATETIME__BUF_SZ)
    {
        ex_set(e, EX_VALUE_ERROR, "invalid datetime format (wrong size)");
        return NULL;
    }

    with_offset = fmt->data[fmt->n-2] == '%' && fmt->data[fmt->n-2] == 'z';

    memcpy(datetime__buf, fmt->data, fmt->n);
    datetime__buf[fmt->n] = '\0';

    return datetime__from_str(str, datetime__buf, with_offset, e);
}

/*
 * Returns `0` when failed (with `e` set), otherwise the number of chars
 * written inside the given buffer.
 */
static size_t datetime__write(
        ti_datetime_t * dt,
        char * buf,
        size_t buf_sz,
        const char * fmt,
        ex_t * e)
{
    size_t sz;
    struct tm tm = {0};
    long int offset = ((long int) dt->offset) * 60;
    time_t ts = dt->ts;

    if ((offset > 0 && ts > LLONG_MAX - offset) ||
        (offset < 0 && ts < LLONG_MIN - offset))
    {
        ex_set(e, EX_OVERFLOW, "datetime overflow");
        return 0;
    }

    ts += offset;

    if (gmtime_r(&ts, &tm) != &tm)
    {
        ex_set(e, EX_VALUE_ERROR,
                "failed to convert to Coordinated Universal Time (UTC)");
        return 0;
    }

    tm.tm_gmtoff = offset;

    sz = strftime(buf, buf_sz, fmt, &tm);
    if (sz == 0)
        ex_set(e, EX_VALUE_ERROR, "invalid datetime template");
    return sz;
}

/*
 * This function is not thread safe.
 */
ti_raw_t * ti_datetime_to_str(ti_datetime_t * dt, ex_t * e)
{
    ti_raw_t * str;
    size_t sz = datetime__write(
            dt,
            datetime__buf,
            DATETIME__BUF_SZ,
            dt->offset ? datetime__fmt_zone : datetime__fmp_utc,
            e);
    if (!sz)
        return NULL;  /* e is set */

    str = ti_str_create(datetime__buf, sz);
    if (!str)
        ex_set_mem(e);
    return str;
}

/*
 * This function is not thread safe.
 */
int ti_datetime_to_pk(ti_datetime_t * dt, msgpack_packer * pk, int options)
{
    if (options >= 0)
    {
        /* pack client result, convert to string */
        ex_t e = {0};
        size_t sz = datetime__write(
                dt,
                datetime__buf,
                DATETIME__BUF_SZ,
                dt->offset ? datetime__fmt_zone : datetime__fmp_utc,
                &e);
        if (sz)
            return mp_pack_strn(pk, datetime__buf, sz);
        else  /* fall-back to time-stamp 0 on conversion error */
            return mp_pack_str(pk, "1970-01-01T00:00:00Z");
    }

    return (
        msgpack_pack_map(pk, 1) ||
        mp_pack_strn(pk, TI_KIND_S_DATETIME, 1) ||
        msgpack_pack_array(pk, 2) ||
        msgpack_pack_int64(pk, dt->ts) ||
        msgpack_pack_int16(pk, dt->offset)
    );
}

/*
 * The following `enum` and `asso_values` are generated using `gperf` and
 * the utility `pcregrep` to generate the input.
 *
 * Command:
 *
 *    pcregrep -o1 ' :: `([\w\/]+)' datetime.c | gperf -E -k '*,1,$' -m 200

  :: `localtime`
  :: `Africa/Abidjan`
  :: `Africa/Accra`
  :: `Africa/Addis_Ababa`
  :: `Africa/Algiers`
  :: `Africa/Asmara`
  :: `Africa/Bamako`
  :: `Africa/Bangui`
  :: `Africa/Banjul`
  :: `Africa/Bissau`
  :: `Africa/Blantyre`
  :: `Africa/Brazzaville`
  :: `Africa/Bujumbura`
  :: `Africa/Cairo`
  :: `Africa/Casablanca`
  :: `Africa/Ceuta`
  :: `Africa/Conakry`
  :: `Africa/Dakar`
  :: `Africa/Dar_es_Salaam`
  :: `Africa/Djibouti`
  :: `Africa/Douala`
  :: `Africa/El_Aaiun`
  :: `Africa/Freetown`
  :: `Africa/Gaborone`
  :: `Africa/Harare`
  :: `Africa/Johannesburg`
  :: `Africa/Juba`
  :: `Africa/Kampala`
  :: `Africa/Khartoum`
  :: `Africa/Kigali`
  :: `Africa/Kinshasa`
  :: `Africa/Lagos`
  :: `Africa/Libreville`
  :: `Africa/Lome`
  :: `Africa/Luanda`
  :: `Africa/Lubumbashi`
  :: `Africa/Lusaka`
  :: `Africa/Malabo`
  :: `Africa/Maputo`
  :: `Africa/Maseru`
  :: `Africa/Mbabane`
  :: `Africa/Mogadishu`
  :: `Africa/Monrovia`
  :: `Africa/Nairobi`
  :: `Africa/Ndjamena`
  :: `Africa/Niamey`
  :: `Africa/Nouakchott`
  :: `Africa/Ouagadougou`
  :: `Africa/Porto-Novo`
  :: `Africa/Sao_Tome`
  :: `Africa/Tripoli`
  :: `Africa/Tunis`
  :: `Africa/Windhoek`
  :: `America/Adak`
  :: `America/Anchorage`
  :: `America/Anguilla`
  :: `America/Antigua`
  :: `America/Araguaina`
  :: `America/Argentina/Buenos_Aires`
  :: `America/Argentina/Catamarca`
  :: `America/Argentina/Cordoba`
  :: `America/Argentina/Jujuy`
  :: `America/Argentina/La_Rioja`
  :: `America/Argentina/Mendoza`
  :: `America/Argentina/Rio_Gallegos`
  :: `America/Argentina/Salta`
  :: `America/Argentina/San_Juan`
  :: `America/Argentina/San_Luis`
  :: `America/Argentina/Tucuman`
  :: `America/Argentina/Ushuaia`
  :: `America/Aruba`
  :: `America/Asuncion`
  :: `America/Atikokan`
  :: `America/Bahia`
  :: `America/Bahia_Banderas`
  :: `America/Barbados`
  :: `America/Belem`
  :: `America/Belize`
  :: `America/Blanc-Sablon`
  :: `America/Boa_Vista`
  :: `America/Bogota`
  :: `America/Boise`
  :: `America/Cambridge_Bay`
  :: `America/Campo_Grande`
  :: `America/Cancun`
  :: `America/Caracas`
  :: `America/Cayenne`
  :: `America/Cayman`
  :: `America/Chicago`
  :: `America/Chihuahua`
  :: `America/Costa_Rica`
  :: `America/Creston`
  :: `America/Cuiaba`
  :: `America/Curacao`
  :: `America/Danmarkshavn`
  :: `America/Dawson`
  :: `America/Dawson_Creek`
  :: `America/Denver`
  :: `America/Detroit`
  :: `America/Dominica`
  :: `America/Edmonton`
  :: `America/Eirunepe`
  :: `America/El_Salvador`
  :: `America/Fort_Nelson`
  :: `America/Fortaleza`
  :: `America/Glace_Bay`
  :: `America/Godthab`
  :: `America/Goose_Bay`
  :: `America/Grand_Turk`
  :: `America/Grenada`
  :: `America/Guadeloupe`
  :: `America/Guatemala`
  :: `America/Guayaquil`
  :: `America/Guyana`
  :: `America/Halifax`
  :: `America/Havana`
  :: `America/Hermosillo`
  :: `America/Indiana/Indianapolis`
  :: `America/Indiana/Knox`
  :: `America/Indiana/Marengo`
  :: `America/Indiana/Petersburg`
  :: `America/Indiana/Tell_City`
  :: `America/Indiana/Vevay`
  :: `America/Indiana/Vincennes`
  :: `America/Indiana/Winamac`
  :: `America/Inuvik`
  :: `America/Iqaluit`
  :: `America/Jamaica`
  :: `America/Juneau`
  :: `America/Kentucky/Louisville`
  :: `America/Kentucky/Monticello`
  :: `America/Kralendijk`
  :: `America/La_Paz`
  :: `America/Lima`
  :: `America/Los_Angeles`
  :: `America/Lower_Princes`
  :: `America/Maceio`
  :: `America/Managua`
  :: `America/Manaus`
  :: `America/Marigot`
  :: `America/Martinique`
  :: `America/Matamoros`
  :: `America/Mazatlan`
  :: `America/Menominee`
  :: `America/Merida`
  :: `America/Metlakatla`
  :: `America/Mexico_City`
  :: `America/Miquelon`
  :: `America/Moncton`
  :: `America/Monterrey`
  :: `America/Montevideo`
  :: `America/Montserrat`
  :: `America/Nassau`
  :: `America/New_York`
  :: `America/Nipigon`
  :: `America/Nome`
  :: `America/Noronha`
  :: `America/North_Dakota/Beulah`
  :: `America/North_Dakota/Center`
  :: `America/North_Dakota/New_Salem`
  :: `America/Ojinaga`
  :: `America/Panama`
  :: `America/Pangnirtung`
  :: `America/Paramaribo`
  :: `America/Phoenix`
  :: `America/Port-au-Prince`
  :: `America/Port_of_Spain`
  :: `America/Porto_Velho`
  :: `America/Puerto_Rico`
  :: `America/Rainy_River`
  :: `America/Rankin_Inlet`
  :: `America/Recife`
  :: `America/Regina`
  :: `America/Resolute`
  :: `America/Rio_Branco`
  :: `America/Santa_Isabel`
  :: `America/Santarem`
  :: `America/Santiago`
  :: `America/Santo_Domingo`
  :: `America/Sao_Paulo`
  :: `America/Scoresbysund`
  :: `America/Sitka`
  :: `America/St_Barthelemy`
  :: `America/St_Johns`
  :: `America/St_Kitts`
  :: `America/St_Lucia`
  :: `America/St_Thomas`
  :: `America/St_Vincent`
  :: `America/Swift_Current`
  :: `America/Tegucigalpa`
  :: `America/Thule`
  :: `America/Thunder_Bay`
  :: `America/Tijuana`
  :: `America/Toronto`
  :: `America/Tortola`
  :: `America/Vancouver`
  :: `America/Whitehorse`
  :: `America/Winnipeg`
  :: `America/Yakutat`
  :: `America/Yellowknife`
  :: `Antarctica/Casey`
  :: `Antarctica/Davis`
  :: `Antarctica/DumontDUrville`
  :: `Antarctica/Macquarie`
  :: `Antarctica/Mawson`
  :: `Antarctica/McMurdo`
  :: `Antarctica/Palmer`
  :: `Antarctica/Rothera`
  :: `Antarctica/Syowa`
  :: `Antarctica/Troll`
  :: `Antarctica/Vostok`
  :: `Arctic/Longyearbyen`
  :: `Asia/Aden`
  :: `Asia/Almaty`
  :: `Asia/Amman`
  :: `Asia/Anadyr`
  :: `Asia/Aqtau`
  :: `Asia/Aqtobe`
  :: `Asia/Ashgabat`
  :: `Asia/Baghdad`
  :: `Asia/Bahrain`
  :: `Asia/Baku`
  :: `Asia/Bangkok`
  :: `Asia/Beirut`
  :: `Asia/Bishkek`
  :: `Asia/Brunei`
  :: `Asia/Chita`
  :: `Asia/Choibalsan`
  :: `Asia/Colombo`
  :: `Asia/Damascus`
  :: `Asia/Dhaka`
  :: `Asia/Dili`
  :: `Asia/Dubai`
  :: `Asia/Dushanbe`
  :: `Asia/Gaza`
  :: `Asia/Hebron`
  :: `Asia/Ho_Chi_Minh`
  :: `Asia/Hong_Kong`
  :: `Asia/Hovd`
  :: `Asia/Irkutsk`
  :: `Asia/Jakarta`
  :: `Asia/Jayapura`
  :: `Asia/Jerusalem`
  :: `Asia/Kabul`
  :: `Asia/Kamchatka`
  :: `Asia/Karachi`
  :: `Asia/Kathmandu`
  :: `Asia/Khandyga`
  :: `Asia/Kolkata`
  :: `Asia/Krasnoyarsk`
  :: `Asia/Kuala_Lumpur`
  :: `Asia/Kuching`
  :: `Asia/Kuwait`
  :: `Asia/Macau`
  :: `Asia/Magadan`
  :: `Asia/Makassar`
  :: `Asia/Manila`
  :: `Asia/Muscat`
  :: `Asia/Nicosia`
  :: `Asia/Novokuznetsk`
  :: `Asia/Novosibirsk`
  :: `Asia/Omsk`
  :: `Asia/Oral`
  :: `Asia/Phnom_Penh`
  :: `Asia/Pontianak`
  :: `Asia/Pyongyang`
  :: `Asia/Qatar`
  :: `Asia/Qyzylorda`
  :: `Asia/Rangoon`
  :: `Asia/Riyadh`
  :: `Asia/Sakhalin`
  :: `Asia/Samarkand`
  :: `Asia/Seoul`
  :: `Asia/Shanghai`
  :: `Asia/Singapore`
  :: `Asia/Srednekolymsk`
  :: `Asia/Taipei`
  :: `Asia/Tashkent`
  :: `Asia/Tbilisi`
  :: `Asia/Tehran`
  :: `Asia/Thimphu`
  :: `Asia/Tokyo`
  :: `Asia/Ulaanbaatar`
  :: `Asia/Urumqi`
  :: `Asia/Ust-Nera`
  :: `Asia/Vientiane`
  :: `Asia/Vladivostok`
  :: `Asia/Yakutsk`
  :: `Asia/Yekaterinburg`
  :: `Asia/Yerevan`
  :: `Atlantic/Azores`
  :: `Atlantic/Bermuda`
  :: `Atlantic/Canary`
  :: `Atlantic/Cape_Verde`
  :: `Atlantic/Faroe`
  :: `Atlantic/Madeira`
  :: `Atlantic/Reykjavik`
  :: `Atlantic/South_Georgia`
  :: `Atlantic/St_Helena`
  :: `Atlantic/Stanley`
  :: `Australia/Adelaide`
  :: `Australia/Brisbane`
  :: `Australia/Broken_Hill`
  :: `Australia/Currie`
  :: `Australia/Darwin`
  :: `Australia/Eucla`
  :: `Australia/Hobart`
  :: `Australia/Lindeman`
  :: `Australia/Lord_Howe`
  :: `Australia/Melbourne`
  :: `Australia/Perth`
  :: `Australia/Sydney`
  :: `Canada/Atlantic`
  :: `Canada/Central`
  :: `Canada/Eastern`
  :: `Canada/Mountain`
  :: `Canada/Newfoundland`
  :: `Canada/Pacific`
  :: `Europe/Amsterdam`
  :: `Europe/Andorra`
  :: `Europe/Athens`
  :: `Europe/Belgrade`
  :: `Europe/Berlin`
  :: `Europe/Bratislava`
  :: `Europe/Brussels`
  :: `Europe/Bucharest`
  :: `Europe/Budapest`
  :: `Europe/Busingen`
  :: `Europe/Chisinau`
  :: `Europe/Copenhagen`
  :: `Europe/Dublin`
  :: `Europe/Gibraltar`
  :: `Europe/Guernsey`
  :: `Europe/Helsinki`
  :: `Europe/Isle_of_Man`
  :: `Europe/Istanbul`
  :: `Europe/Jersey`
  :: `Europe/Kaliningrad`
  :: `Europe/Kiev`
  :: `Europe/Lisbon`
  :: `Europe/Ljubljana`
  :: `Europe/London`
  :: `Europe/Luxembourg`
  :: `Europe/Madrid`
  :: `Europe/Malta`
  :: `Europe/Mariehamn`
  :: `Europe/Minsk`
  :: `Europe/Monaco`
  :: `Europe/Moscow`
  :: `Europe/Oslo`
  :: `Europe/Paris`
  :: `Europe/Podgorica`
  :: `Europe/Prague`
  :: `Europe/Riga`
  :: `Europe/Rome`
  :: `Europe/Samara`
  :: `Europe/San_Marino`
  :: `Europe/Sarajevo`
  :: `Europe/Simferopol`
  :: `Europe/Skopje`
  :: `Europe/Sofia`
  :: `Europe/Stockholm`
  :: `Europe/Tallinn`
  :: `Europe/Tirane`
  :: `Europe/Uzhgorod`
  :: `Europe/Vaduz`
  :: `Europe/Vatican`
  :: `Europe/Vienna`
  :: `Europe/Vilnius`
  :: `Europe/Volgograd`
  :: `Europe/Warsaw`
  :: `Europe/Zagreb`
  :: `Europe/Zaporozhye`
  :: `Europe/Zurich`
  :: `GMT`
  :: `Indian/Antananarivo`
  :: `Indian/Chagos`
  :: `Indian/Christmas`
  :: `Indian/Cocos`
  :: `Indian/Comoro`
  :: `Indian/Kerguelen`
  :: `Indian/Mahe`
  :: `Indian/Maldives`
  :: `Indian/Mauritius`
  :: `Indian/Mayotte`
  :: `Indian/Reunion`
  :: `Pacific/Apia`
  :: `Pacific/Auckland`
  :: `Pacific/Bougainville`
  :: `Pacific/Chatham`
  :: `Pacific/Chuuk`
  :: `Pacific/Easter`
  :: `Pacific/Efate`
  :: `Pacific/Enderbury`
  :: `Pacific/Fakaofo`
  :: `Pacific/Fiji`
  :: `Pacific/Funafuti`
  :: `Pacific/Galapagos`
  :: `Pacific/Gambier`
  :: `Pacific/Guadalcanal`
  :: `Pacific/Guam`
  :: `Pacific/Honolulu`
  :: `Pacific/Johnston`
  :: `Pacific/Kiritimati`
  :: `Pacific/Kosrae`
  :: `Pacific/Kwajalein`
  :: `Pacific/Majuro`
  :: `Pacific/Marquesas`
  :: `Pacific/Midway`
  :: `Pacific/Nauru`
  :: `Pacific/Niue`
  :: `Pacific/Norfolk`
  :: `Pacific/Noumea`
  :: `Pacific/Pago_Pago`
  :: `Pacific/Palau`
  :: `Pacific/Pitcairn`
  :: `Pacific/Pohnpei`
  :: `Pacific/Port_Moresby`
  :: `Pacific/Rarotonga`
  :: `Pacific/Saipan`
  :: `Pacific/Tahiti`
  :: `Pacific/Tarawa`
  :: `Pacific/Tongatapu`
  :: `Pacific/Wake`
  :: `Pacific/Wallis`
  :: `US/Alaska`
  :: `US/Arizona`
  :: `US/Central`
  :: `US/Eastern`
  :: `US/Hawaii`
  :: `US/Mountain`
  :: `US/Pacific`
  :: `UTC`

 */

enum
{
    TOTAL_KEYWORDS = 433,
    MIN_WORD_LENGTH = 3,
    MAX_WORD_LENGTH = 30,
    MIN_HASH_VALUE = 22,
    MAX_HASH_VALUE = 1849
};

static unsigned int datetime__hash(
          register const char * s,
          register size_t n)
{
    static unsigned short asso_values[] =
    {
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850,   61, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850,    9,   89,   66,  403,    3,
        289,  223,  363,  106,  483,  241,  321,    0,  341,  286,
        15,    0,  293,  152,  101,  370,  435,  471, 1850,   10,
        14, 1850, 1850, 1850, 1850,   51, 1850,    0,  147,    1,
        37,    0,   14,   40,   50,    1,   35,  106,    3,    0,
         0,    4,   28,  484,    0,    0,    1,    3,  213,  178,
        82,  103,  320, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850, 1850,
        1850, 1850, 1850, 1850, 1850, 1850
    };
    register unsigned int hval = n;

    switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)s[29]];
      /*fall through*/
      case 29:
        hval += asso_values[(unsigned char)s[28]];
      /*fall through*/
      case 28:
        hval += asso_values[(unsigned char)s[27]];
      /*fall through*/
      case 27:
        hval += asso_values[(unsigned char)s[26]];
      /*fall through*/
      case 26:
        hval += asso_values[(unsigned char)s[25]];
      /*fall through*/
      case 25:
        hval += asso_values[(unsigned char)s[24]];
      /*fall through*/
      case 24:
        hval += asso_values[(unsigned char)s[23]];
      /*fall through*/
      case 23:
        hval += asso_values[(unsigned char)s[22]];
      /*fall through*/
      case 22:
        hval += asso_values[(unsigned char)s[21]];
      /*fall through*/
      case 21:
        hval += asso_values[(unsigned char)s[20]];
      /*fall through*/
      case 20:
        hval += asso_values[(unsigned char)s[19]];
      /*fall through*/
      case 19:
        hval += asso_values[(unsigned char)s[18]];
      /*fall through*/
      case 18:
        hval += asso_values[(unsigned char)s[17]];
      /*fall through*/
      case 17:
        hval += asso_values[(unsigned char)s[16]];
      /*fall through*/
      case 16:
        hval += asso_values[(unsigned char)s[15]];
      /*fall through*/
      case 15:
        hval += asso_values[(unsigned char)s[14]];
      /*fall through*/
      case 14:
        hval += asso_values[(unsigned char)s[13]];
      /*fall through*/
      case 13:
        hval += asso_values[(unsigned char)s[12]];
      /*fall through*/
      case 12:
        hval += asso_values[(unsigned char)s[11]];
      /*fall through*/
      case 11:
        hval += asso_values[(unsigned char)s[10]];
      /*fall through*/
      case 10:
        hval += asso_values[(unsigned char)s[9]];
      /*fall through*/
      case 9:
        hval += asso_values[(unsigned char)s[8]];
      /*fall through*/
      case 8:
        hval += asso_values[(unsigned char)s[7]];
      /*fall through*/
      case 7:
        hval += asso_values[(unsigned char)s[6]];
      /*fall through*/
      case 6:
        hval += asso_values[(unsigned char)s[5]];
      /*fall through*/
      case 5:
        hval += asso_values[(unsigned char)s[4]];
      /*fall through*/
      case 4:
        hval += asso_values[(unsigned char)s[3]];
      /*fall through*/
      case 3:
        hval += asso_values[(unsigned char)s[2]];
      /*fall through*/
      case 2:
        hval += asso_values[(unsigned char)s[1]];
      /*fall through*/
      case 1:
        hval += asso_values[(unsigned char)s[0]];
        break;
    }
  return hval;
}

_Bool ti_datetime_is_timezone(register const char * s, register size_t n)
{
    static const char * wordlist[] =
    {
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "",
        "localtime",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "",
        "Asia/Qatar",
        "", "",
        "Asia/Macau",
        "Asia/Manila",
        "Asia/Muscat",
        "",
        "America/Manaus",
        "Asia/Amman",
        "",
        "America/Maceio",
        "",
        "America/Menominee",
        "",
        "America/Montserrat",
        "America/Moncton",
        "America/Matamoros",
        "", "",
        "America/Panama",
        "Africa/Maseru",
        "Australia/Eucla",
        "America/Port",
        "",
        "America/Asuncion",
        "",
        "Africa/Asmara",
        "Africa/Accra",
        "Antarctica/Palmer",
        "",
        "Pacific/Easter",
        "", "",
        "Europe/Malta",
        "", "", "", "", "",
        "Europe/Monaco",
        "Africa/Porto",
        "America/Eirunepe",
        "America/Merida",
        "Pacific/Efate",
        "Asia/Aden",
        "Europe/Paris",
        "Pacific/Palau",
        "Pacific/Pitcairn",
        "America/Managua",
        "Atlantic/Madeira",
        "",
        "America/Marigot",
        "",
        "Africa/Maputo",
        "",
        "America/Edmonton",
        "Antarctica/McMurdo",
        "", "",
        "America/Antigua",
        "America/Araguaina",
        "",
        "Pacific/Apia",
        "", "",
        "America/Anguilla",
        "", "",
        "Pacific/Majuro",
        "", "",
        "Africa/Algiers",
        "America/Caracas",
        "",
        "America/Cancun",
        "",
        "America/Creston",
        "Australia/Perth",
        "Asia/Magadan",
        "America/Curacao",
        "Europe/Amsterdam",
        "Europe/Andorra",
        "Australia/Currie",
        "",
        "Europe/Mariehamn",
        "",
        "Africa/Ceuta",
        "Africa/Cairo",
        "Europe/Prague",
        "Africa/El_Aaiun",
        "Europe/Athens",
        "", "",
        "Asia/Brunei",
        "Asia/Beirut",
        "America/Belem",
        "America/Blanc",
        "America/Boise",
        "",
        "Europe/Zurich",
        "Canada/Eastern",
        "Australia/Adelaide",
        "", "", "",
        "Europe/Madrid",
        "Canada/Mountain",
        "",
        "Asia/Makassar",
        "America/Pangnirtung",
        "Africa/Bissau",
        "America/Anchorage",
        "",
        "Canada/Atlantic",
        "",
        "America/Monterrey",
        "Asia/Almaty",
        "Asia/Chita",
        "America/Tortola",
        "America/Toronto",
        "Antarctica/Troll",
        "Africa/Tunis",
        "America/Metlakatla",
        "Europe/Berlin",
        "",
        "Pacific/Pohnpei",
        "America/Yakutat",
        "Europe/Brussels",
        "",
        "Canada/Pacific",
        "Asia/Pontianak",
        "Asia/Taipei",
        "Europe/Tirane",
        "", "",
        "Europe/Podgorica",
        "Europe/Minsk",
        "", "",
        "Europe/Tallinn",
        "Atlantic/Bermuda",
        "Asia/Bahrain",
        "America/Bogota",
        "America/Bahia",
        "",
        "America/Tijuana",
        "",
        "Africa/Banjul",
        "Indian/Mauritius",
        "Asia/Anadyr",
        "Africa/Bangui",
        "Asia/Tehran",
        "",
        "Europe/Chisinau",
        "America/Adak",
        "Africa/Mogadishu",
        "Africa/Tripoli",
        "America/Phoenix",
        "",
        "America/Santarem",
        "America/Thule",
        "Asia/Seoul",
        "America/Aruba",
        "", "",
        "Europe/Busingen",
        "Canada/Central",
        "America/Chicago",
        "", "", "",
        "Africa/Malabo",
        "Australia/Melbourne",
        "America/Cayman",
        "America/Cayenne",
        "America/Paramaribo",
        "",
        "Europe/Bucharest",
        "Antarctica/Casey",
        "Atlantic/Canary",
        "Pacific/Tahiti",
        "",
        "Europe/Samara",
        "",
        "Indian/Mahe",
        "", "",
        "Pacific/Auckland",
        "",
        "Asia/Phnom_Penh",
        "Europe/Budapest",
        "",
        "Antarctica/Mawson",
        "",
        "Pacific/Chatham",
        "",
        "Asia/Baku",
        "America/Argentina/Catamarca",
        "Pacific/Pago_Pago",
        "",
        "Europe/Sofia",
        "Europe/Belgrade",
        "", "",
        "America/Santiago",
        "",
        "Pacific/Tongatapu",
        "Pacific/Saipan",
        "", "",
        "Indian/Cocos",
        "", "", "",
        "Indian/Comoro",
        "Africa/Blantyre",
        "Africa/Bamako",
        "Europe/Moscow",
        "", "", "",
        "America/Cuiaba",
        "Europe/Copenhagen",
        "",
        "Asia/Yerevan",
        "",
        "America/Tegucigalpa",
        "Asia/Yakutsk",
        "Asia/Singapore",
        "Asia/Colombo",
        "America/Chihuahua",
        "Europe/Zagreb",
        "",
        "America/Atikokan",
        "Asia/Thimphu",
        "America/Argentina/Tucuman",
        "",
        "America/Guatemala",
        "Africa/Casablanca",
        "America/Sao_Paulo",
        "Europe/Simferopol",
        "Africa/Monrovia",
        "Europe/San_Marino",
        "", "",
        "GMT",
        "Indian/Mayotte",
        "Africa/Abidjan",
        "",
        "Asia/Ashgabat",
        "Pacific/Guam",
        "Australia/Brisbane",
        "",
        "Pacific/Chuuk",
        "Asia/Baghdad",
        "Asia/Tbilisi",
        "",
        "Indian/Christmas",
        "", "",
        "Asia/Tashkent",
        "",
        "America/Indiana/Marengo",
        "America/Sitka",
        "Europe/Isle_of_Man",
        "America/Grenada",
        "", "",
        "America/Montevideo",
        "",
        "Atlantic/Stanley",
        "Pacific/Kosrae",
        "", "", "",
        "Asia/Choibalsan",
        "",
        "Pacific/Kiritimati",
        "", "", "",
        "America/Argentina/Salta",
        "",
        "America/Barbados",
        "", "", "",
        "Asia/Oral",
        "",
        "America/Argentina/Buenos_Aires",
        "Africa/Kampala",
        "",
        "Europe/Istanbul",
        "",
        "Asia/Karachi",
        "Asia/Shanghai",
        "Indian/Chagos",
        "Africa/Conakry",
        "Asia/Samarkand",
        "",
        "Africa/Bujumbura",
        "Pacific/Guadalcanal",
        "Atlantic/Faroe",
        "Africa/Kigali",
        "",
        "Pacific/Tarawa",
        "", "",
        "Asia/Pyongyang",
        "America/Guadeloupe",
        "America/Resolute",
        "Africa/Kinshasa",
        "Africa/Mbabane",
        "America/Recife",
        "Asia/Sakhalin",
        "Asia/Yekaterinburg",
        "",
        "Asia/Tokyo",
        "Africa/Khartoum",
        "America/Mexico_City",
        "",
        "Europe/Oslo",
        "Pacific/Enderbury",
        "Asia/Irkutsk",
        "America/Lima",
        "Europe/Rome",
        "",
        "Pacific/Galapagos",
        "America/Yellowknife",
        "America/Bahia_Banderas",
        "America/Mazatlan",
        "Africa/Sao_Tome",
        "America/Port_of_Spain",
        "America/Guyana",
        "",
        "Asia/Kathmandu",
        "",
        "Asia/Kuching",
        "America/Regina",
        "Pacific/Funafuti",
        "Africa/Lome",
        "Europe/Bratislava",
        "Asia/Rangoon",
        "Atlantic/Azores",
        "",
        "Pacific/Midway",
        "Asia/Bangkok",
        "America/Nome",
        "America/Nassau",
        "Asia/Nicosia",
        "Pacific/Fiji",
        "", "",
        "Asia/Bishkek",
        "Europe/Stockholm",
        "Europe/Skopje",
        "Asia/Kolkata",
        "Pacific/Port_Moresby",
        "Europe/Kaliningrad",
        "Antarctica/Rothera",
        "",
        "Europe/Guernsey",
        "Europe/Riga",
        "", "", "",
        "America/St_Thomas",
        "America/Ojinaga",
        "Asia/Ust",
        "Pacific/Niue",
        "Indian/Antananarivo",
        "Pacific/Rarotonga",
        "Pacific/Nauru",
        "Australia/Lindeman",
        "Pacific/Noumea",
        "", "", "",
        "Africa/Luanda",
        "",
        "Africa/Harare",
        "Africa/Lagos",
        "America/Puerto_Rico",
        "", "",
        "America/Argentina/Cordoba",
        "America/Hermosillo",
        "", "",
        "Pacific/Bougainville",
        "Asia/Omsk",
        "",
        "Indian/Maldives",
        "Asia/Kabul",
        "", "",
        "Europe/London",
        "Africa/Gaborone",
        "Pacific/Gambier",
        "", "", "",
        "Asia/Kamchatka",
        "America/Indiana/Indianapolis",
        "America/Noronha",
        "",
        "Asia/Dili",
        "Australia/Sydney",
        "Europe/Gibraltar",
        "Asia/Damascus",
        "",
        "Pacific/Honolulu",
        "", "", "",
        "America/Detroit",
        "America/Dominica",
        "America/Belize",
        "",
        "America/Campo_Grande",
        "America/Nipigon",
        "", "", "",
        "Asia/Kuwait",
        "America/Costa_Rica",
        "Indian/Kerguelen",
        "America/Indiana/Petersburg",
        "", "",
        "Africa/Douala",
        "America/Kralendijk",
        "Africa/Ndjamena",
        "America/Inuvik",
        "", "",
        "Europe/Sarajevo",
        "America/Los_Angeles",
        "Indian/Reunion",
        "", "",
        "Asia/Vientiane",
        "Africa/Ouagadougou",
        "America/Thunder_Bay",
        "Pacific/Fakaofo",
        "",
        "Antarctica/Syowa",
        "Africa/Lusaka",
        "", "", "",
        "America/Rio_Branco",
        "", "",
        "America/St_Kitts",
        "", "",
        "America/Scoresbysund",
        "UTC",
        "Asia/Krasnoyarsk",
        "Africa/Addis_Ababa",
        "America/St_Barthelemy",
        "Africa/Niamey",
        "", "", "",
        "Europe/Vienna",
        "America/Godthab",
        "America/Halifax",
        "Europe/Vatican",
        "America/Santa_Isabel",
        "", "",
        "Asia/Khandyga",
        "Europe/Vilnius",
        "America/El_Salvador",
        "",
        "America/Glace_Bay",
        "America/Swift_Current",
        "", "",
        "America/Goose_Bay",
        "",
        "Europe/Kiev",
        "Asia/Riyadh",
        "", "",
        "Pacific/Kwajalein",
        "America/Argentina/Mendoza",
        "",
        "America/Jamaica",
        "Africa/Freetown",
        "Asia/Jerusalem",
        "America/Juneau",
        "", "",
        "Asia/Aqtau",
        "",
        "America/Martinique",
        "Pacific/Norfolk",
        "",
        "America/Miquelon",
        "Antarctica/Macquarie",
        "Europe/Lisbon",
        "Pacific/Wallis",
        "",
        "Europe/Helsinki",
        "", "", "", "", "",
        "Africa/Nairobi",
        "",
        "Asia/Hebron",
        "US/Eastern",
        "Pacific/Marquesas",
        "",
        "Asia/Srednekolymsk",
        "", "",
        "US/Mountain",
        "", "", "",
        "Africa/Dakar",
        "Asia/Ulaanbaatar",
        "Australia/Hobart",
        "",
        "America/Grand_Turk",
        "", "",
        "Africa/Nouakchott",
        "", "", "",
        "America/St_Lucia",
        "", "", "", "",
        "Asia/Gaza",
        "America/Indiana/Knox",
        "",
        "US/Pacific",
        "America/Cambridge_Bay",
        "Atlantic/South_Georgia",
        "America/Winnipeg",
        "",
        "America/Indiana/Tell_City",
        "America/Kentucky/Monticello",
        "America/Argentina/Ushuaia",
        "",
        "Asia/Dubai",
        "", "", "",
        "Europe/Zaporozhye",
        "Asia/Dhaka",
        "",
        "Antarctica/Vostok",
        "", "", "", "", "", "", "", "", "",
        "Pacific/Johnston",
        "America/Rankin_Inlet",
        "",
        "Asia/Qyzylorda",
        "", "",
        "America/Porto_Velho",
        "Europe/Ljubljana",
        "", "",
        "America/Havana",
        "US/Central",
        "America/Lower_Princes",
        "Atlantic/St_Helena",
        "",
        "America/Whitehorse",
        "",
        "Europe/Dublin",
        "America/Boa_Vista",
        "America/Dawson",
        "",
        "Asia/Jakarta",
        "", "",
        "Australia/Darwin",
        "",
        "Europe/Volgograd",
        "", "", "", "",
        "Pacific/Wake",
        "",
        "America/Iqaluit",
        "",
        "Asia/Dushanbe",
        "", "", "", "", "", "", "", "",
        "Africa/Djibouti",
        "Asia/Hovd",
        "Europe/Jersey",
        "", "",
        "Asia/Jayapura",
        "America/Denver",
        "", "", "",
        "America/Fortaleza",
        "",
        "Antarctica/Davis",
        "",
        "US/Alaska",
        "", "",
        "Atlantic/Cape_Verde",
        "", "",
        "Europe/Luxembourg",
        "", "",
        "Australia/Broken_Hill",
        "", "", "", "",
        "Asia/Ho_Chi_Minh",
        "", "",
        "Asia/Aqtobe",
        "", "",
        "Africa/Juba",
        "",
        "America/St_Vincent",
        "", "", "", "", "",
        "America/Argentina/San_Luis",
        "America/Indiana/Vincennes",
        "",
        "Asia/Kuala_Lumpur",
        "", "", "",
        "America/Vancouver",
        "", "", "", "", "", "", "",
        "America/Santo_Domingo",
        "", "", "", "", "", "", "",
        "Europe/Warsaw",
        "", "", "", "",
        "Africa/Dar_es_Salaam",
        "", "", "",
        "Africa/Windhoek",
        "", "",
        "America/Indiana/Winamac",
        "",
        "Africa/Lubumbashi",
        "", "",
        "America/New_York",
        "", "", "", "", "",
        "America/Fort_Nelson",
        "", "", "", "", "", "", "",
        "Africa/Libreville",
        "America/La_Paz",
        "", "", "", "", "", "",
        "Canada/Newfoundland",
        "", "", "", "", "", "", "", "", "",
        "",
        "Arctic/Longyearbyen",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "",
        "Asia/Hong_Kong",
        "America/St_Johns",
        "", "",
        "Africa/Johannesburg",
        "",
        "America/Argentina/Jujuy",
        "",
        "America/Argentina/Rio_Gallegos",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "America/Danmarkshavn",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "",
        "Asia/Vladivostok",
        "", "", "", "", "", "", "",
        "America/Argentina/San_Juan",
        "America/Dawson_Creek",
        "", "", "",
        "Asia/Novosibirsk",
        "",
        "Europe/Vaduz",
        "", "",
        "America/Guayaquil",
        "", "", "", "", "",
        "America/Argentina/La_Rioja",
        "", "", "", "", "", "", "", "", "",
        "", "",
        "US/Arizona",
        "", "", "", "", "", "", "", "", "",
        "", "",
        "Europe/Uzhgorod",
        "Asia/Urumqi",
        "", "", "", "", "", "", "", "", "",
        "", "",
        "Atlantic/Reykjavik",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "",
        "America/Rainy_River",
        "", "",
        "America/Indiana/Vevay",
        "", "", "",
        "Africa/Brazzaville",
        "",
        "Australia/Lord_Howe",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "",
        "US/Hawaii",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "",
        "America/Kentucky/Louisville",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "",
        "Asia/Novokuznetsk",
        "",
        "America/North_Dakota/Center",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "",
        "America/North_Dakota/Beulah",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "",
        "Antarctica/DumontDUrville",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "",
        "", "",
        "America/North_Dakota/New_Salem"
    };

    if (n <= MAX_WORD_LENGTH && n >= MIN_WORD_LENGTH)
    {
        register unsigned int key = datetime__hash (s, n);
        if (key <= MAX_HASH_VALUE)
        {
            register const char * ws = wordlist[key];

            if (strlen(ws) == n && !memcmp(s, ws, n))
                return true;
        }
    }
    return false;
}

int ti_datetime_to_zone(ti_datetime_t * dt, ti_raw_t * str, ex_t * e)
{
    long int offset;

    if (!str->n)
    {
        ex_set(e, EX_VALUE_ERROR,
                "expecting a timezone or offset; "
                "example timezone: `Europe/Amsterdam`; example offset: `+01`");
        return e->nr;
    }

    if (*str->data != '+' && *str->data != '-')
    {
        struct tm tm;
        char buf[MAX_WORD_LENGTH+5];

        if (!ti_datetime_is_timezone((const char *) str->data, str->n))
        {
            ex_set(e, EX_VALUE_ERROR, "unknown timezone");
            return e->nr;
        }

        memcpy(buf, "TZ=:", 4);
        memcpy(buf+4, str->data, str->n);
        buf[str->n+4] = '\0';

        (void) putenv(buf);
        tzset();

        if (!*tzname[1])
        {
            ex_set(e, EX_VALUE_ERROR, "failed to set timezone: `%.*s`",
                    (int) str->n,
                    (const char *) str->data);
            return e->nr;
        }

        /* localize the time-stamp so we get the correct offset including
         * daylight saving time.
         */
        memset(&tm, 0, sizeof(struct tm));
        if (localtime_r(&dt->ts, &tm) != &tm)
        {
            ex_set(e, EX_VALUE_ERROR, "failed to localize datetime");
            return e->nr;
        }

        offset = tm.tm_gmtoff;
    }
    else
        offset = ti_datetime_offset_in_sec(str, e);

    offset /= 60;
    dt->offset = offset;

    return e->nr;
}

int ti_datetime_localize(ti_datetime_t * dt, ex_t * e)
{
    long int offset = dt->offset * 60;

    /* on error, offset is 0 so just continue */
    if ((offset < 0 && dt->ts > LLONG_MAX + offset) ||
        (offset > 0 && dt->ts < LLONG_MIN + offset))
    {
        ex_set(e, EX_OVERFLOW, "datetime overflow");
        return e->nr;
    }

    dt->ts -= offset;
    return 0;
}
