#!/usr/bin/env python
import asyncio
import pickle
import time
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


class TestBackup(TestBase):

    title = 'Test backup'

    @default_test_setup(
        num_nodes=2, seed=1,
        threshold_full_storage=100,
        gcloud_key_file='/tmp/_test_ti.json')
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('/n/0')

        # add another node otherwise backups are not possible
        await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_new_backup(self, client):

        with self.assertRaisesRegex(
                TypeError,
                r'function `new_backup` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('new_backup(123);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `new_backup` requires at least 1 argument '
                'but 0 were given'):
            await client.query('new_backup();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `new_backup` takes at most 4 arguments '
                'but 5 were given'):
            await client.query('new_backup("a", 2, 3, 4, 5);')

        backup_id = await client.query(r'''
            new_backup('/tmp/test.tar.gz');
        ''')

        self.assertTrue(isinstance(backup_id, int))

    async def test_has_backup(self, client):
        backup_id = await client.query(r'''
            new_backup('/tmp/test.tar.gz');
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has_backup`'):
            await client.query('nil.has_backup();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_backup` takes 1 argument but 0 were given'):
            await client.query('has_backup();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has_backup` expects argument 1 to be of '
                r'type `int` but got type `str` instead'):
            await client.query('has_backup("/tmp");')

        with self.assertRaisesRegex(
                ValueError,
                r'function `has_backup` expects argument 1 to be an '
                r'integer value greater than or equal to 0'):
            await client.query('has_backup(-1);')

        self.assertTrue(await client.query(f'''has_backup({backup_id});'''))
        self.assertFalse(await client.query(r'''has_backup(1234);'''))

    async def test_restore(self, client):
        await client.query(r''' .foo = 'bar'; ''', scope='//stuff')
        await client.query(r'''new_backup('/tmp/test.tar.gz');''')

        # in 50 seconds both nodes should have been in `away` mode
        await asyncio.sleep(50)

        await client.query(r'''del_collection('stuff');''', scope='@t')

        # in 50 seconds both nodes should have been in `away` mode
        await asyncio.sleep(50)

        await client.query(r'''restore('/tmp/test.tar.gz');''', scope='@t')

        client.close()
        await client.wait_closed()

        # in 30 seconds synchronization should have been finished
        await asyncio.sleep(30)

        client = await get_client(self.node0)
        bar = await client.query('.foo;', scope='//stuff')

        self.assertEqual(bar, 'bar')

    async def test_error_gcs(self, client):
        # bug #288
        backup_id = await client.query("""//ti
            new_backup('gs://some_bucket/some_file.tar.gz');
        """)

        # in 50 seconds both nodes should have been in `away` mode
        await asyncio.sleep(50)

        res = await client.query("""//ti
            backup_info(backup_id);
        """, backup_id=backup_id)

        self.assertNotEqual(res['result_code'], 0)


if __name__ == '__main__':
    run_test(TestBackup())
