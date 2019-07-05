/*
 * util/iso8601.h - Library to parse ISO 8601 dates.
 */
#ifndef ISO8601_H_
#define ISO8601_H_

#include <inttypes.h>
#include <stddef.h>

/*
 * Returns a time-stamp in seconds for a given date or a negative value in
 * case or an error.
 */
int64_t iso8601_parse_date(const char * str);
int64_t iso8601_parse_date_n(const char * str, size_t n);

#endif  /* ISO8601_H_ */
