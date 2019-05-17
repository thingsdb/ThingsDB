#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.target import Target
from thingsdb.exceptions import IndexError
from thingsdb.exceptions import BadRequestError


class TestCollectionFunctions(TestBase):

    title = 'Test collection functions'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.test_remove(client)

        client.close()
        await client.wait_closed()

    async def test_remove(self, client):
        await client.query('list = [1, 2, 3];')
        self.assertEqual((await client.query('list.remove(|x|(x>1));')), 2)
        self.assertEqual((await client.query('list.remove(|x|(x>1));')), 3)
        self.assertEqual((await client.query('list;')), [1])
        self.assertEqual((await client.query('list.remove(||false);')), None)
        self.assertEqual((await client.query('list.remove(||false, "");')), '')
        self.assertEqual((await client.query('[].remove(||true);')), None)
        self.assertEqual((await client.query('["pi"].remove(||true);')), 'pi')

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `remove`'):
            await client.query('nil.remove(||true);')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `remove`'):
            await client.query('a = [list]; a[0].remove(||true);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `remove` requires at least 1 argument '
                'but 0 were given'):
            await client.query('list.remove();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `remove` expects at most 2 arguments '
                'but 3 were given'):
            await client.query('list.remove(||true, 2, 3);')

        with self.assertRaisesRegex(
                BadRequestError,
                'cannot use function `remove` while the list is in use'):
            await client.query('list.map(||list.remove(||true));')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `remove` expects a `closure` '
                'but got type `nil` instead'):
            await client.query('list.remove(nil);')


if __name__ == '__main__':
    run_test(TestCollectionFunctions())
