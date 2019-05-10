"""Sets the following variable:
THINGSDB_BIN
    Path to ThingsDB executable (binary).
THINGSDB_TESTDIR
    Path used for generated test files. The directory will be removed or
    overwritten by each test, so be carefull.
THINGSDB_MEMCHECK
    Enable or disable the `valgrind` memory checker. Valid options are
    0 (disabled), 1 (basic) or 2 (full). Default value is 1 (basic).
THINGSDB_NODE_OUTPUT
    When set, node output will be written to stdout. If the value can be parsed
    to an integer value, only `that` node output will be written.
THINGSDB_LOGLEVEL
    Loglevel used for testing. Default log is `critical`.
THINGSDB_VERBOSE
    Sets ThingsDB log level set to `debug` instead of `info`
"""

import sys
import os
import logging
import time

THINGSDB_BIN = os.environ.get('THINGSDB_BIN', '../Debug/thingsdb')
if not THINGSDB_BIN.startswith('/'):
    THINGSDB_BIN = os.path.join(os.getcwd(), THINGSDB_BIN)

if not os.path.isfile(THINGSDB_BIN) or not os.access(THINGSDB_BIN, os.X_OK):
    sys.exit(f'THINGSDB_BIN ({THINGSDB_BIN}) is not an executable file')

THINGSDB_TESTDIR = os.environ.get('THINGSDB_TESTDIR', './testdir')
if not THINGSDB_TESTDIR.startswith('/'):
    THINGSDB_TESTDIR = os.path.join(os.getcwd(), THINGSDB_TESTDIR)

try:
    THINGSDB_MEMCHECK = [
        # memcheck disabled
        [],
        # basic memcheck
        [
            'valgrind',
            '--tool=memcheck',
            '--error-exitcode=1',
            '--leak-check=full',
        ],
        # full and vebose memcheck
        [
            'valgrind',
            '--tool=memcheck',
            '--error-exitcode=1',
            '--leak-check=full',
            '--show-leak-kinds=all',
            '--track-origins=yes',
            '-v',
        ],
    ][int(os.environ.get('THINGSDB_MEMCHECK', 1))]

except ValueError:
    sys.exit('THINGSDB_MEMCHECK shoud be an integer value')
except IndexError:
    sys.exit(
        'THINGSDB_MEMCHECK shoud be one of '
        '0 (disabled), 1 (basic) or 2 (full)'
        f'got `{THINGSDB_MEMCHECK}`'
    )

THINGSDB_NODE_OUTPUT = os.environ.get('THINGSDB_NODE_OUTPUT', None)
if THINGSDB_NODE_OUTPUT is not None:
    try:
        THINGSDB_NODE_OUTPUT = int(THINGSDB_NODE_OUTPUT)
    except ValueError:
        THINGSDB_NODE_OUTPUT = True

THINGSDB_LOGLEVEL = os.environ.get('THINGSDB_LOGLEVEL', 'critical').upper()

if not hasattr(logging, THINGSDB_LOGLEVEL):
    sys.exit(
        f'unknown THINGSDB_LOGLEVEL: `{THINGSDB_LOGLEVEL}`'
    )

THINGSDB_VERBOSE = os.environ.get('THINGSDB_VERBOSE', None)
if THINGSDB_VERBOSE is not None:
    if THINGSDB_VERBOSE == '0' or THINGSDB_VERBOSE.lower() == 'false':
        THINGSDB_VERBOSE = None

THINGSDB_VERBOSE = bool(THINGSDB_VERBOSE)
