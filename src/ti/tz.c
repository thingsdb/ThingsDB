/*
 * ti/tz.c
 */
#include <assert.h>
#include <ti/tz.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ti/raw.inline.h>
#include <util/logger.h>
#include <util/mpack.h>

enum
{
    TOTAL_KEYWORDS = 593,
    MIN_WORD_LENGTH = 2,
    MAX_WORD_LENGTH = 32,
    MIN_HASH_VALUE = 13,
    MAX_HASH_VALUE = 2688
};


static inline unsigned int tz__hash(
        register const char * s,
        register size_t n)
{
    static unsigned short asso_values[] =
    {
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689,  113, 2689,   64,    2,    2,   38,    2,
          16,   78,   36,   34,   26,   22,    9,   35, 2689, 2689,
        2689, 2689, 2689, 2689, 2689,   10,  136,    2,  463,   13,
         451,  196,  730,  366,  641,  484,  640,    6,  466,   41,
          13,    8,  520,  213,  195,  396,  483,   54, 2689,  291,
         147, 2689, 2689, 2689, 2689,  170, 2689,    3,   13,    2,
          39,    2,    4,   95,   49,    8,    2,  182,   44,    3,
           3,    3,   43,  358,    2,    4,    2,    2,  174,  361,
           2,  304,  250,    7, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689, 2689,
        2689, 2689, 2689, 2689, 2689, 2689, 2689
    };

    register unsigned int hval = n;

    switch (hval)
    {
        default:
            hval += asso_values[(unsigned char)s[31]];
            /*fall through*/
        case 31:
            hval += asso_values[(unsigned char)s[30]];
            /*fall through*/
        case 30:
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
            hval += asso_values[(unsigned char)s[6]+1];
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
            hval += asso_values[(unsigned char)s[2]+1];
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


/*
 * DO NOT MANUALLY EDIT THE LIST BELOW, ORDER IS IMPORTANT!!
 *
 * Use `gen_tz_mapping.py` instead, and if changes detected, re-run the code
 * below:
 *
 *   pcregrep -o1 '\.name\=\"([\w\_\/\-]+)' tz.c | gperf -E -k '*,1,$' -m 200
 */
static ti_tz_t tz__list[TOTAL_KEYWORDS] = {
    {.name="UTC"},
    {.name="Europe/Amsterdam"},
    {.name="Africa/Abidjan"},
    {.name="Africa/Accra"},
    {.name="Africa/Addis_Ababa"},
    {.name="Africa/Algiers"},
    {.name="Africa/Asmara"},
    {.name="Africa/Bamako"},
    {.name="Africa/Bangui"},
    {.name="Africa/Banjul"},
    {.name="Africa/Bissau"},
    {.name="Africa/Blantyre"},
    {.name="Africa/Brazzaville"},
    {.name="Africa/Bujumbura"},
    {.name="Africa/Cairo"},
    {.name="Africa/Casablanca"},
    {.name="Africa/Ceuta"},
    {.name="Africa/Conakry"},
    {.name="Africa/Dakar"},
    {.name="Africa/Dar_es_Salaam"},
    {.name="Africa/Djibouti"},
    {.name="Africa/Douala"},
    {.name="Africa/El_Aaiun"},
    {.name="Africa/Freetown"},
    {.name="Africa/Gaborone"},
    {.name="Africa/Harare"},
    {.name="Africa/Johannesburg"},
    {.name="Africa/Juba"},
    {.name="Africa/Kampala"},
    {.name="Africa/Khartoum"},
    {.name="Africa/Kigali"},
    {.name="Africa/Kinshasa"},
    {.name="Africa/Lagos"},
    {.name="Africa/Libreville"},
    {.name="Africa/Lome"},
    {.name="Africa/Luanda"},
    {.name="Africa/Lubumbashi"},
    {.name="Africa/Lusaka"},
    {.name="Africa/Malabo"},
    {.name="Africa/Maputo"},
    {.name="Africa/Maseru"},
    {.name="Africa/Mbabane"},
    {.name="Africa/Mogadishu"},
    {.name="Africa/Monrovia"},
    {.name="Africa/Nairobi"},
    {.name="Africa/Ndjamena"},
    {.name="Africa/Niamey"},
    {.name="Africa/Nouakchott"},
    {.name="Africa/Ouagadougou"},
    {.name="Africa/Porto-Novo"},
    {.name="Africa/Sao_Tome"},
    {.name="Africa/Tripoli"},
    {.name="Africa/Tunis"},
    {.name="Africa/Windhoek"},
    {.name="America/Adak"},
    {.name="America/Anchorage"},
    {.name="America/Anguilla"},
    {.name="America/Antigua"},
    {.name="America/Araguaina"},
    {.name="America/Argentina/Buenos_Aires"},
    {.name="America/Argentina/Catamarca"},
    {.name="America/Argentina/Cordoba"},
    {.name="America/Argentina/Jujuy"},
    {.name="America/Argentina/La_Rioja"},
    {.name="America/Argentina/Mendoza"},
    {.name="America/Argentina/Rio_Gallegos"},
    {.name="America/Argentina/Salta"},
    {.name="America/Argentina/San_Juan"},
    {.name="America/Argentina/San_Luis"},
    {.name="America/Argentina/Tucuman"},
    {.name="America/Argentina/Ushuaia"},
    {.name="America/Aruba"},
    {.name="America/Asuncion"},
    {.name="America/Atikokan"},
    {.name="America/Bahia"},
    {.name="America/Bahia_Banderas"},
    {.name="America/Barbados"},
    {.name="America/Belem"},
    {.name="America/Belize"},
    {.name="America/Blanc-Sablon"},
    {.name="America/Boa_Vista"},
    {.name="America/Bogota"},
    {.name="America/Boise"},
    {.name="America/Cambridge_Bay"},
    {.name="America/Campo_Grande"},
    {.name="America/Cancun"},
    {.name="America/Caracas"},
    {.name="America/Cayenne"},
    {.name="America/Cayman"},
    {.name="America/Chicago"},
    {.name="America/Chihuahua"},
    {.name="America/Costa_Rica"},
    {.name="America/Creston"},
    {.name="America/Cuiaba"},
    {.name="America/Curacao"},
    {.name="America/Danmarkshavn"},
    {.name="America/Dawson"},
    {.name="America/Dawson_Creek"},
    {.name="America/Denver"},
    {.name="America/Detroit"},
    {.name="America/Dominica"},
    {.name="America/Edmonton"},
    {.name="America/Eirunepe"},
    {.name="America/El_Salvador"},
    {.name="America/Fort_Nelson"},
    {.name="America/Fortaleza"},
    {.name="America/Glace_Bay"},
    {.name="America/Goose_Bay"},
    {.name="America/Grand_Turk"},
    {.name="America/Grenada"},
    {.name="America/Guadeloupe"},
    {.name="America/Guatemala"},
    {.name="America/Guayaquil"},
    {.name="America/Guyana"},
    {.name="America/Halifax"},
    {.name="America/Havana"},
    {.name="America/Hermosillo"},
    {.name="America/Indiana/Indianapolis"},
    {.name="America/Indiana/Knox"},
    {.name="America/Indiana/Marengo"},
    {.name="America/Indiana/Petersburg"},
    {.name="America/Indiana/Tell_City"},
    {.name="America/Indiana/Vevay"},
    {.name="America/Indiana/Vincennes"},
    {.name="America/Indiana/Winamac"},
    {.name="America/Inuvik"},
    {.name="America/Iqaluit"},
    {.name="America/Jamaica"},
    {.name="America/Juneau"},
    {.name="America/Kentucky/Louisville"},
    {.name="America/Kentucky/Monticello"},
    {.name="America/Kralendijk"},
    {.name="America/La_Paz"},
    {.name="America/Lima"},
    {.name="America/Los_Angeles"},
    {.name="America/Lower_Princes"},
    {.name="America/Maceio"},
    {.name="America/Managua"},
    {.name="America/Manaus"},
    {.name="America/Marigot"},
    {.name="America/Martinique"},
    {.name="America/Matamoros"},
    {.name="America/Mazatlan"},
    {.name="America/Menominee"},
    {.name="America/Merida"},
    {.name="America/Metlakatla"},
    {.name="America/Mexico_City"},
    {.name="America/Miquelon"},
    {.name="America/Moncton"},
    {.name="America/Monterrey"},
    {.name="America/Montevideo"},
    {.name="America/Montserrat"},
    {.name="America/Nassau"},
    {.name="America/New_York"},
    {.name="America/Nipigon"},
    {.name="America/Nome"},
    {.name="America/Noronha"},
    {.name="America/North_Dakota/Beulah"},
    {.name="America/North_Dakota/Center"},
    {.name="America/North_Dakota/New_Salem"},
    {.name="America/Nuuk"},
    {.name="America/Ojinaga"},
    {.name="America/Panama"},
    {.name="America/Pangnirtung"},
    {.name="America/Paramaribo"},
    {.name="America/Phoenix"},
    {.name="America/Port-au-Prince"},
    {.name="America/Port_of_Spain"},
    {.name="America/Porto_Velho"},
    {.name="America/Puerto_Rico"},
    {.name="America/Punta_Arenas"},
    {.name="America/Rainy_River"},
    {.name="America/Rankin_Inlet"},
    {.name="America/Recife"},
    {.name="America/Regina"},
    {.name="America/Resolute"},
    {.name="America/Rio_Branco"},
    {.name="America/Santarem"},
    {.name="America/Santiago"},
    {.name="America/Santo_Domingo"},
    {.name="America/Sao_Paulo"},
    {.name="America/Scoresbysund"},
    {.name="America/Sitka"},
    {.name="America/St_Barthelemy"},
    {.name="America/St_Johns"},
    {.name="America/St_Kitts"},
    {.name="America/St_Lucia"},
    {.name="America/St_Thomas"},
    {.name="America/St_Vincent"},
    {.name="America/Swift_Current"},
    {.name="America/Tegucigalpa"},
    {.name="America/Thule"},
    {.name="America/Thunder_Bay"},
    {.name="America/Tijuana"},
    {.name="America/Toronto"},
    {.name="America/Tortola"},
    {.name="America/Vancouver"},
    {.name="America/Whitehorse"},
    {.name="America/Winnipeg"},
    {.name="America/Yakutat"},
    {.name="America/Yellowknife"},
    {.name="Antarctica/Casey"},
    {.name="Antarctica/Davis"},
    {.name="Antarctica/DumontDUrville"},
    {.name="Antarctica/Macquarie"},
    {.name="Antarctica/Mawson"},
    {.name="Antarctica/McMurdo"},
    {.name="Antarctica/Palmer"},
    {.name="Antarctica/Rothera"},
    {.name="Antarctica/Syowa"},
    {.name="Antarctica/Troll"},
    {.name="Antarctica/Vostok"},
    {.name="Arctic/Longyearbyen"},
    {.name="Asia/Aden"},
    {.name="Asia/Almaty"},
    {.name="Asia/Amman"},
    {.name="Asia/Anadyr"},
    {.name="Asia/Aqtau"},
    {.name="Asia/Aqtobe"},
    {.name="Asia/Ashgabat"},
    {.name="Asia/Atyrau"},
    {.name="Asia/Baghdad"},
    {.name="Asia/Bahrain"},
    {.name="Asia/Baku"},
    {.name="Asia/Bangkok"},
    {.name="Asia/Barnaul"},
    {.name="Asia/Beirut"},
    {.name="Asia/Bishkek"},
    {.name="Asia/Brunei"},
    {.name="Asia/Chita"},
    {.name="Asia/Choibalsan"},
    {.name="Asia/Colombo"},
    {.name="Asia/Damascus"},
    {.name="Asia/Dhaka"},
    {.name="Asia/Dili"},
    {.name="Asia/Dubai"},
    {.name="Asia/Dushanbe"},
    {.name="Asia/Famagusta"},
    {.name="Asia/Gaza"},
    {.name="Asia/Hebron"},
    {.name="Asia/Ho_Chi_Minh"},
    {.name="Asia/Hong_Kong"},
    {.name="Asia/Hovd"},
    {.name="Asia/Irkutsk"},
    {.name="Asia/Jakarta"},
    {.name="Asia/Jayapura"},
    {.name="Asia/Jerusalem"},
    {.name="Asia/Kabul"},
    {.name="Asia/Kamchatka"},
    {.name="Asia/Karachi"},
    {.name="Asia/Kathmandu"},
    {.name="Asia/Khandyga"},
    {.name="Asia/Kolkata"},
    {.name="Asia/Krasnoyarsk"},
    {.name="Asia/Kuala_Lumpur"},
    {.name="Asia/Kuching"},
    {.name="Asia/Kuwait"},
    {.name="Asia/Macau"},
    {.name="Asia/Magadan"},
    {.name="Asia/Makassar"},
    {.name="Asia/Manila"},
    {.name="Asia/Muscat"},
    {.name="Asia/Nicosia"},
    {.name="Asia/Novokuznetsk"},
    {.name="Asia/Novosibirsk"},
    {.name="Asia/Omsk"},
    {.name="Asia/Oral"},
    {.name="Asia/Phnom_Penh"},
    {.name="Asia/Pontianak"},
    {.name="Asia/Pyongyang"},
    {.name="Asia/Qatar"},
    {.name="Asia/Qostanay"},
    {.name="Asia/Qyzylorda"},
    {.name="Asia/Riyadh"},
    {.name="Asia/Sakhalin"},
    {.name="Asia/Samarkand"},
    {.name="Asia/Seoul"},
    {.name="Asia/Shanghai"},
    {.name="Asia/Singapore"},
    {.name="Asia/Srednekolymsk"},
    {.name="Asia/Taipei"},
    {.name="Asia/Tashkent"},
    {.name="Asia/Tbilisi"},
    {.name="Asia/Tehran"},
    {.name="Asia/Thimphu"},
    {.name="Asia/Tokyo"},
    {.name="Asia/Tomsk"},
    {.name="Asia/Ulaanbaatar"},
    {.name="Asia/Urumqi"},
    {.name="Asia/Ust-Nera"},
    {.name="Asia/Vientiane"},
    {.name="Asia/Vladivostok"},
    {.name="Asia/Yakutsk"},
    {.name="Asia/Yangon"},
    {.name="Asia/Yekaterinburg"},
    {.name="Asia/Yerevan"},
    {.name="Atlantic/Azores"},
    {.name="Atlantic/Bermuda"},
    {.name="Atlantic/Canary"},
    {.name="Atlantic/Cape_Verde"},
    {.name="Atlantic/Faroe"},
    {.name="Atlantic/Madeira"},
    {.name="Atlantic/Reykjavik"},
    {.name="Atlantic/South_Georgia"},
    {.name="Atlantic/St_Helena"},
    {.name="Atlantic/Stanley"},
    {.name="Australia/Adelaide"},
    {.name="Australia/Brisbane"},
    {.name="Australia/Broken_Hill"},
    {.name="Australia/Currie"},
    {.name="Australia/Darwin"},
    {.name="Australia/Eucla"},
    {.name="Australia/Hobart"},
    {.name="Australia/Lindeman"},
    {.name="Australia/Lord_Howe"},
    {.name="Australia/Melbourne"},
    {.name="Australia/Perth"},
    {.name="Australia/Sydney"},
    {.name="Canada/Atlantic"},
    {.name="Canada/Central"},
    {.name="Canada/Eastern"},
    {.name="Canada/Mountain"},
    {.name="Canada/Newfoundland"},
    {.name="Canada/Pacific"},
    {.name="Europe/Andorra"},
    {.name="Europe/Astrakhan"},
    {.name="Europe/Athens"},
    {.name="Europe/Belgrade"},
    {.name="Europe/Berlin"},
    {.name="Europe/Bratislava"},
    {.name="Europe/Brussels"},
    {.name="Europe/Bucharest"},
    {.name="Europe/Budapest"},
    {.name="Europe/Busingen"},
    {.name="Europe/Chisinau"},
    {.name="Europe/Copenhagen"},
    {.name="Europe/Dublin"},
    {.name="Europe/Gibraltar"},
    {.name="Europe/Guernsey"},
    {.name="Europe/Helsinki"},
    {.name="Europe/Isle_of_Man"},
    {.name="Europe/Istanbul"},
    {.name="Europe/Jersey"},
    {.name="Europe/Kaliningrad"},
    {.name="Europe/Kiev"},
    {.name="Europe/Kirov"},
    {.name="Europe/Lisbon"},
    {.name="Europe/Ljubljana"},
    {.name="Europe/London"},
    {.name="Europe/Luxembourg"},
    {.name="Europe/Madrid"},
    {.name="Europe/Malta"},
    {.name="Europe/Mariehamn"},
    {.name="Europe/Minsk"},
    {.name="Europe/Monaco"},
    {.name="Europe/Moscow"},
    {.name="Europe/Oslo"},
    {.name="Europe/Paris"},
    {.name="Europe/Podgorica"},
    {.name="Europe/Prague"},
    {.name="Europe/Riga"},
    {.name="Europe/Rome"},
    {.name="Europe/Samara"},
    {.name="Europe/San_Marino"},
    {.name="Europe/Sarajevo"},
    {.name="Europe/Saratov"},
    {.name="Europe/Simferopol"},
    {.name="Europe/Skopje"},
    {.name="Europe/Sofia"},
    {.name="Europe/Stockholm"},
    {.name="Europe/Tallinn"},
    {.name="Europe/Tirane"},
    {.name="Europe/Ulyanovsk"},
    {.name="Europe/Uzhgorod"},
    {.name="Europe/Vaduz"},
    {.name="Europe/Vatican"},
    {.name="Europe/Vienna"},
    {.name="Europe/Vilnius"},
    {.name="Europe/Volgograd"},
    {.name="Europe/Warsaw"},
    {.name="Europe/Zagreb"},
    {.name="Europe/Zaporozhye"},
    {.name="Europe/Zurich"},
    {.name="GMT"},
    {.name="Indian/Antananarivo"},
    {.name="Indian/Chagos"},
    {.name="Indian/Christmas"},
    {.name="Indian/Cocos"},
    {.name="Indian/Comoro"},
    {.name="Indian/Kerguelen"},
    {.name="Indian/Mahe"},
    {.name="Indian/Maldives"},
    {.name="Indian/Mauritius"},
    {.name="Indian/Mayotte"},
    {.name="Indian/Reunion"},
    {.name="Pacific/Apia"},
    {.name="Pacific/Auckland"},
    {.name="Pacific/Bougainville"},
    {.name="Pacific/Chatham"},
    {.name="Pacific/Chuuk"},
    {.name="Pacific/Easter"},
    {.name="Pacific/Efate"},
    {.name="Pacific/Enderbury"},
    {.name="Pacific/Fakaofo"},
    {.name="Pacific/Fiji"},
    {.name="Pacific/Funafuti"},
    {.name="Pacific/Galapagos"},
    {.name="Pacific/Gambier"},
    {.name="Pacific/Guadalcanal"},
    {.name="Pacific/Guam"},
    {.name="Pacific/Honolulu"},
    {.name="Pacific/Kiritimati"},
    {.name="Pacific/Kosrae"},
    {.name="Pacific/Kwajalein"},
    {.name="Pacific/Majuro"},
    {.name="Pacific/Marquesas"},
    {.name="Pacific/Midway"},
    {.name="Pacific/Nauru"},
    {.name="Pacific/Niue"},
    {.name="Pacific/Norfolk"},
    {.name="Pacific/Noumea"},
    {.name="Pacific/Pago_Pago"},
    {.name="Pacific/Palau"},
    {.name="Pacific/Pitcairn"},
    {.name="Pacific/Pohnpei"},
    {.name="Pacific/Port_Moresby"},
    {.name="Pacific/Rarotonga"},
    {.name="Pacific/Saipan"},
    {.name="Pacific/Tahiti"},
    {.name="Pacific/Tarawa"},
    {.name="Pacific/Tongatapu"},
    {.name="Pacific/Wake"},
    {.name="Pacific/Wallis"},
    {.name="US/Alaska"},
    {.name="US/Arizona"},
    {.name="US/Central"},
    {.name="US/Eastern"},
    {.name="US/Hawaii"},
    {.name="US/Mountain"},
    {.name="US/Pacific"},
    {.name="Africa/Asmera"},
    {.name="Africa/Timbuktu"},
    {.name="America/Argentina/ComodRivadavia"},
    {.name="America/Atka"},
    {.name="America/Buenos_Aires"},
    {.name="America/Catamarca"},
    {.name="America/Coral_Harbour"},
    {.name="America/Cordoba"},
    {.name="America/Ensenada"},
    {.name="America/Fort_Wayne"},
    {.name="America/Godthab"},
    {.name="America/Indianapolis"},
    {.name="America/Jujuy"},
    {.name="America/Knox_IN"},
    {.name="America/Louisville"},
    {.name="America/Mendoza"},
    {.name="America/Montreal"},
    {.name="America/Porto_Acre"},
    {.name="America/Rosario"},
    {.name="America/Santa_Isabel"},
    {.name="America/Shiprock"},
    {.name="America/Virgin"},
    {.name="Antarctica/South_Pole"},
    {.name="Asia/Ashkhabad"},
    {.name="Asia/Calcutta"},
    {.name="Asia/Chongqing"},
    {.name="Asia/Chungking"},
    {.name="Asia/Dacca"},
    {.name="Asia/Harbin"},
    {.name="Asia/Istanbul"},
    {.name="Asia/Kashgar"},
    {.name="Asia/Katmandu"},
    {.name="Asia/Macao"},
    {.name="Asia/Rangoon"},
    {.name="Asia/Saigon"},
    {.name="Asia/Tel_Aviv"},
    {.name="Asia/Thimbu"},
    {.name="Asia/Ujung_Pandang"},
    {.name="Asia/Ulan_Bator"},
    {.name="Atlantic/Faeroe"},
    {.name="Atlantic/Jan_Mayen"},
    {.name="Australia/ACT"},
    {.name="Australia/Canberra"},
    {.name="Australia/LHI"},
    {.name="Australia/NSW"},
    {.name="Australia/North"},
    {.name="Australia/Queensland"},
    {.name="Australia/South"},
    {.name="Australia/Tasmania"},
    {.name="Australia/Victoria"},
    {.name="Australia/West"},
    {.name="Australia/Yancowinna"},
    {.name="Brazil/Acre"},
    {.name="Brazil/DeNoronha"},
    {.name="Brazil/East"},
    {.name="Brazil/West"},
    {.name="CET"},
    {.name="CST6CDT"},
    {.name="Canada/Saskatchewan"},
    {.name="Canada/Yukon"},
    {.name="Chile/Continental"},
    {.name="Chile/EasterIsland"},
    {.name="Cuba"},
    {.name="EET"},
    {.name="EST"},
    {.name="EST5EDT"},
    {.name="Egypt"},
    {.name="Eire"},
    {.name="Etc/GMT"},
    {.name="Etc/GMT+0"},
    {.name="Etc/GMT+1"},
    {.name="Etc/GMT+10"},
    {.name="Etc/GMT+11"},
    {.name="Etc/GMT+12"},
    {.name="Etc/GMT+2"},
    {.name="Etc/GMT+3"},
    {.name="Etc/GMT+4"},
    {.name="Etc/GMT+5"},
    {.name="Etc/GMT+6"},
    {.name="Etc/GMT+7"},
    {.name="Etc/GMT+8"},
    {.name="Etc/GMT+9"},
    {.name="Etc/GMT-0"},
    {.name="Etc/GMT-1"},
    {.name="Etc/GMT-10"},
    {.name="Etc/GMT-11"},
    {.name="Etc/GMT-12"},
    {.name="Etc/GMT-13"},
    {.name="Etc/GMT-14"},
    {.name="Etc/GMT-2"},
    {.name="Etc/GMT-3"},
    {.name="Etc/GMT-4"},
    {.name="Etc/GMT-5"},
    {.name="Etc/GMT-6"},
    {.name="Etc/GMT-7"},
    {.name="Etc/GMT-8"},
    {.name="Etc/GMT-9"},
    {.name="Etc/GMT0"},
    {.name="Etc/Greenwich"},
    {.name="Etc/UCT"},
    {.name="Etc/UTC"},
    {.name="Etc/Universal"},
    {.name="Etc/Zulu"},
    {.name="Europe/Belfast"},
    {.name="Europe/Nicosia"},
    {.name="Europe/Tiraspol"},
    {.name="GB"},
    {.name="GB-Eire"},
    {.name="GMT+0"},
    {.name="GMT-0"},
    {.name="GMT0"},
    {.name="Greenwich"},
    {.name="HST"},
    {.name="Hongkong"},
    {.name="Iceland"},
    {.name="Iran"},
    {.name="Israel"},
    {.name="Jamaica"},
    {.name="Japan"},
    {.name="Kwajalein"},
    {.name="Libya"},
    {.name="MET"},
    {.name="MST"},
    {.name="MST7MDT"},
    {.name="Mexico/BajaNorte"},
    {.name="Mexico/BajaSur"},
    {.name="Mexico/General"},
    {.name="NZ"},
    {.name="NZ-CHAT"},
    {.name="Navajo"},
    {.name="PRC"},
    {.name="PST8PDT"},
    {.name="Pacific/Johnston"},
    {.name="Pacific/Ponape"},
    {.name="Pacific/Samoa"},
    {.name="Pacific/Truk"},
    {.name="Pacific/Yap"},
    {.name="Poland"},
    {.name="Portugal"},
    {.name="ROC"},
    {.name="ROK"},
    {.name="Singapore"},
    {.name="Turkey"},
    {.name="UCT"},
    {.name="US/Aleutian"},
    {.name="US/East-Indiana"},
    {.name="US/Indiana-Starke"},
    {.name="US/Michigan"},
    {.name="US/Samoa"},
    {.name="Universal"},
    {.name="W-SU"},
    {.name="WET"},
    {.name="Zulu"},
};


static ti_tz_t * tz__mapping[MAX_HASH_VALUE+1];
static char tz__env[MAX_WORD_LENGTH+5];
static char * tz__write_env = tz__env + 4;

void ti_tz_init(void)
{
    memcpy(tz__env, "TZ=:UTC", 8);
    (void) putenv(tz__env);
    tzset();

    for (size_t i = 0, n = TOTAL_KEYWORDS; i < n; ++i)
    {
        unsigned int key;
        ti_tz_t * tz = &tz__list[i];

        tz->index = i;
        tz->n = strlen(tz->name);
        key = tz__hash(tz->name, tz->n);

        assert (tz__mapping[key] == NULL);
        assert (key <= MAX_HASH_VALUE);

        tz__mapping[key] = tz;
    }
}

void ti_tz_set_utc(void)
{
    memcpy(tz__write_env, "UTC", 4);
}

void ti_tz_set(ti_tz_t * tz)
{
    memcpy(tz__write_env, tz->name, tz->n+1);
}

ti_tz_t * ti_tz_utc(void)
{
    return &tz__list[0];
}

/* Always returns a time zone, UTC if no valid index is given. As this is
 * used internally only. An error will be logged when an invalid time zone
 * is asked.
 */
ti_tz_t * ti_tz_from_index(size_t tz_index)
{
    if (tz_index >= TOTAL_KEYWORDS)
    {
        log_error("unknown time zone index: %zu", tz_index);
        tz_index = 0;
    }
    return &tz__list[tz_index];
}

/*
 * Returns NULL if the time zone is unknown.
 */
ti_tz_t * ti_tz_from_strn(register const char * s, register size_t n)
{
    register unsigned int key;
    ti_tz_t * tz;

    if (n == 0)
        return NULL;

    key = tz__hash(s, n);
    tz = key <= MAX_HASH_VALUE ? tz__mapping[key] : NULL;
    return tz && memcmp(tz->name, s, n) == 0 ? tz : NULL;
}

ti_val_t * ti_tz_as_mpval(void)
{
    ti_raw_t * raw;
    msgpack_packer pk;
    msgpack_sbuffer buffer;

    mp_sbuffer_alloc_init(&buffer, sizeof(ti_raw_t), sizeof(ti_raw_t));
    msgpack_packer_init(&pk, &buffer, msgpack_sbuffer_write);

    if (msgpack_pack_array(&pk, TOTAL_KEYWORDS))
        goto fail;

    for (size_t i = 0, n = TOTAL_KEYWORDS; i < n; ++i)
    {
        ti_tz_t * tz = &tz__list[i];
        if (mp_pack_strn(&pk, tz->name, tz->n))
            goto fail;
    }

    raw = (ti_raw_t *) buffer.data;
    ti_raw_init(raw, TI_VAL_MPDATA, buffer.size);

    return (ti_val_t *) raw;

fail:
    msgpack_sbuffer_destroy(&buffer);
    return NULL;
}
