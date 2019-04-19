"""Sets the following variable:
THINGSDB_BIN
    Path to ThingsDB executable (binary).
THINGSDB_TESTDIR
    Path used for generated test files. The directory will be removed or
    overwritten by each test, so be carefull.
THINGSDB_MEMCHECK
    Enable or disable the `valgrind` memory checker. Valid options are
    0 (disabled), 1 (basic) or 2 (full). Default value is 1 (basic).
THINGSDB_TERMINAL
    When set, nodes will open inside a terminal.
THINGSDB_TERM_KEEP
    Keep the terminal open when a node is turned off.
    (only in case a terminal is used, see THINGSDB_TERMINAL)
THINGSDB_TERM_GEOMETRY
    Set terminal geometry. Default is `140x60`
    (only in case a terminal is used, see THINGSDB_TERMINAL)
THINGSDB_LOGLEVEL
    Loglevel used for testing. Default log is `critical`.
"""

import sys
import os


THINGSDB_BIN = os.environ.get('THINGSDB_BIN', '../Debug/thingsdb')
if not os.path.isfile(THINGSDB_BIN) or not os.access(THINGSDB_BIN, os.X_OK):
    sys.exit(f'THINGSDB_BIN ({THINGSDB_BIN}) is not an executable file')


THINGSDB_TESTDIR = os.environ.get('THINGSDB_TESTDIR', '../testdir')

try:
    THINGSDB_MEMCHECK = [
        # memcheck disabled
        '',
        # basic memcheck
        'valgrind' \
        ' --tool=memcheck'  \
        ' --error-exitcode=1' \
        ' --leak-check=full ',
        # full and vebose memcheck
        'valgrind' \
        ' --tool=memcheck' \
        ' --error-exitcode=1' \
        ' --leak-check=full' \
        ' --show-leak-kinds=all' \
        ' --track-origins=yes' \
        ' -v '
    ][int(os.environ.get('THINGSDB_MEMCHECK', 1))]

except ValueError:
    sys.exit('THINGSDB_MEMCHECK shoud be an integer value')
except IndexError:
    sys.exit(
        'THINGSDB_MEMCHECK shoud be one of '
        '0 (disabled), 1 (basic) or 2 (full)'
        f'got `{THINGSDB_MEMCHECK}`'
    )

try:
    THINGSDB_TERMINAL = os.environ.get('THINGSDB_TERMINAL', None)
    if THINGSDB_TERMINAL == '0' or THINGSDB_TERMINAL == '':
        THINGSDB_TERMINAL = None

try:
    THINGSDB_TERM_KEEP = bool(int(os.environ.get('THINGSDB_TERM_KEEP', 0)))
except ValueError:
    sys.exit('THINGSDB_TERM_KEEP shoud be an integer value')

THINGSDB_TERM_GEOMETRY = os.environ.get('THINGSDB_TERM_GEOMETRY', '140x60')

try:
    _valid_log_levels = ['debug', 'info', 'warning', 'error', 'critical']
    THINGSDB_LOGLEVEL = os.environ.get('THINGSDB_TERMINAL', 'critical').lower()

    if THINGSDB_LOGLEVEL not in _valid_log_levels:
        sys.exit(
            f'THINGSDB_LOGLEVEL should be one of {_valid_log_levels}, '
            f'got `{THINGSDB_LOGLEVEL}`'
        )