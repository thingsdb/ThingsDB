"""Generate Time Zones
This script detects time zones which are specified by pytz library.
Zones which are found in the common list and missing in ThingsDB
will be added, none of the existing time zones will be removed.

Without arguments, this script only displays the changes.
"""
import pytz
import argparse
import pprint
import re

TZ_LINE = re.compile(
    '^\s+\{\.name\=\"([\w\_\/\-\+]+)\"\}\,.*')

def update_info(lines):
    last_line = False
    new_zones = []
    lookup = set()

    for i, line in enumerate(lines):
        m = TZ_LINE.match(line)
        if m:
            last_line = True
            lookup.add(m.group(1))
        elif last_line:
            last_line = i
            break

    if last_line is None:
        raise 'no matching lines are found'

    for zone in pytz.all_timezones:
        if zone not in lookup:
            new_zones.append(f'    {{.name="{zone}"}},\n')

    return last_line, new_zones


if __name__ == '__main__':

    parser = argparse.ArgumentParser()

    parser.add_argument(
        '--apply',
        action='store_true',
        help='add new zones to the source file')

    args = parser.parse_args()

    with open('src/ti/tz.c', 'r') as f:
        lines = f.readlines()

    last_line, zones = update_info(lines)

    if not zones:
        exit('no new zones found')

    if args.apply:
        lines[last_line:last_line] = zones
        with open('src/ti/tz.c', 'w') as f:
            f.write(''.join(lines))
    else:
        pprint.pprint(zones)
        print(f'\n{len(zones)} zone(s) found, use `--apply` to add the zones')

