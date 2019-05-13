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
    killall()


def killall():
    logging.info('Force kill all open thingsdb processes')
    os.system('pkill -9 thingsdb')

    logging.info('Force kill all open memcheck-amd64- processes')
    os.system('pkill -9 memcheck-amd64-')
