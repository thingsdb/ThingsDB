#!/usr/bin/env python
import asyncio
import os
import logging
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client


BACKUP_PATH = os.environ.get('BACKUP_PATH', '')
if not BACKUP_PATH:
    raise Exception('Environment variable BACKUP_PATH is required')


class TestMigrate(TestBase):

    title = 'Test migration'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10)
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        query = client.query

        for fn in os.listdir(BACKUP_PATH):
            if not fn.endswith('.tar.gz'):
                continue

            logging.warning(f"""
    {'#' * 80}

    Next file: {fn}

    {'#' * 80}
""")

            fn = os.path.join(BACKUP_PATH, fn)

            collections = await query('collections_info();')

            for collection in collections:
                await query('del_collection(c);', c=collection['name'])

            await query('restore(fn, {take_access: true});', fn=fn)

            await asyncio.sleep(10)
            await client.authenticate('admin', 'pass')

            logging.warning(f"""
    {'#' * 80}

    Imported file: {fn}, restart test...

    {'#' * 80}
""")

            await self.node0.shutdown()
            await self.node0.run(timeout=20)

            logging.warning(f"""
    {'#' * 80}

    Finished file: {fn}

    {'#' * 80}
""")

        client.close()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestMigrate())
