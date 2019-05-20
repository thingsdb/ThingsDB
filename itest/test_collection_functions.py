#!/usr/bin/env python
import asyncio
import pickle
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.target import Target
from thingsdb.exceptions import IndexError
from thingsdb.exceptions import BadRequestError
from thingsdb.exceptions import AssertionError


class TestCollectionFunctions(TestBase):

    title = 'Test collection functions'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.test_assert(client)
        await self.test_blob(client)
        await self.test_remove(client)

        client.close()
        await client.wait_closed()

    async def test_assert(self, client):
        with self.assertRaisesRegex(
                AssertionError,
                r'assertion statement `\(1>2\)` has failed'):
            await client.query('assert((1>2));')

        try:
            await client.query('assert((1>2), "my custom message", 6);')
        except AssertionError as e:
            self.assertEqual(str(e), 'my custom message')
            self.assertEqual(e.error_code, 6)
        else:
            raise Exception('AssertionError not raised')

        self.assertEqual((await client.query('assert((2>1)); 42;')), 42)

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` requires at least 1 argument '
                'but 0 were given'):
            await client.query('assert();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` expects at most 3 arguments '
                'but 4 were given'):
            await client.query('assert(true, "bla", 1, nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` expects argument 2 to be of '
                'type `raw` but got `nil` instead'):
            await client.query('assert(false, nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` expects a message '
                'to have valid UTF8 encoding'):
            await client.query(
                'assert(false, blob(0));',
                blobs=(pickle.dumps({}), ))

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` expects argument 3 to be of '
                'type `int` but got `nil` instead'):
            await client.query('assert(false, "msg", nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` expects a custom error_code between '
                '1 and 32 but got 0 instead'):
            await client.query('assert(false, "msg", 0);')

    async def test_blob(self, client):
        self.assertEqual((await client.query(
            'objects = [blob(0), blob(1)]; nil;',
            blobs=(pickle.dumps({}), pickle.dumps([])))), None)

        d, a = await client.query('objects;')
        self.assertTrue(isinstance(pickle.loads(d), dict))
        self.assertTrue(isinstance(pickle.loads(a), list))

        with self.assertRaisesRegex(
                BadRequestError,
                'function `blob` takes 1 argument but 0 were given'):
            await client.query('blob();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `blob` expects argument 1 to be of '
                'type `int` but got `nil` instead'):
            await client.query('blob(nil);')

        with self.assertRaisesRegex(
                IndexError, 'blob index out of range'):
            await client.query('blob(0);')

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
