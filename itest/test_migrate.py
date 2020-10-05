#!/usr/bin/env python
import asyncio
import pickle
import time
import os
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import ValueError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import BadDataError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError
from thingsdb.exceptions import OperationError


BACKUP_PATH = os.environ.get('BACKUP_PATH')
if BACKUP_PATH is None:
    raise Exception('Environment variable BACKUP_PATH is required')


class TestMigrate(TestBase):

    title = 'Test simple run'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        query = client.query

        for fn in os.listdir(BACKUP_PATH):
            if not fn.ends_with('.tar.gz'):
                continue

            fn = os.path.join(BACKUP_PATH, fn)

            collections = await query('collections_info();')

            for collection in collections:
                await query('del_collection(c);', c=collection['name'])

            await query('restore(fn, true);', fn=fn)

            await asyncio.sleep(10)
            await client.authenticate('admin', 'pass')

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestMigrate())
