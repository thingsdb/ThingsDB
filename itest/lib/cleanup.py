import os
import logging
import shutil
from .vars import THINGSDB_TESTDIR


def cleanup():
    logging.info('Remove test dir')
    try:
        shutil.rmtree(os.path.join(THINGSDB_TESTDIR))
    except FileNotFoundError:
        pass
    os.mkdir(THINGSDB_TESTDIR)

    logging.info('Force kill all open siridb-server processes')
    os.system('pkill -9 siridb-server')

    logging.info('Force kill all open memcheck-amd64- processes')
    os.system('pkill -9 memcheck-amd64-')
