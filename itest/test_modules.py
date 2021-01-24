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
from thingsdb.exceptions import ForbiddenError


class TestModules(TestBase):

    title = 'Test modules'

    @default_test_setup(
        num_nodes=1,
        seed=1,
        threshold_full_storage=10,
        modules_path='../modules/')
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_new_module(self, client):
        client.set_default_scope('//stuff')
        with self.assertRaisesRegex(
                LookupError,
                r'function `new_module` is undefined in the `@collection` '
                r'scope; you might want to query the `@thingsdb` scope\?'):
            await client.query('new_module("X", "x", nil, nil);')

        client.set_default_scope('/t')
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `new_module` takes 4 arguments but 1 was given'):
            await client.query('new_module(1234);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `new_module` expects argument 1 to be '
                r'of type `str` but got type `int` instead'):
            await client.query('new_module(1, "x", nil, nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'module name must follow the naming rules'):
            await client.query('new_module("", "x", nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `new_module` expects argument 2 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('new_module("X", 1, nil, nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'file argument must not be an empty string'):
            await client.query('new_module("X", "", nil, nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'file argument contains illegal characters'):
            await client.query('new_module("X", "\n", nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `new_module` expects argument 4 to be of '
                r'type `str` or `nil` but got type `int` instead'):
            await client.query('new_module("X", "x", nil, 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; scopes must start with a `@` or `/` but '
                r'got `stuff` instead'):
            await client.query('new_module("X", "x", nil, "stuff");')

        with self.assertRaisesRegex(
                LookupError,
                r'collection `bla` not found'):
            await client.query('new_module("X", "x", nil, "//bla");')

        res = await client.query('new_module("X", "x", nil, "//stuff");')
        self.assertIs(res, None)

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def test_module_info(self, client):
        res = await client.query(r'''
            new_module("X", "x", nil, "//stuff");
        ''', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `module_info` takes 1 argument but 2 were given'):
            await client.query('module_info("X", "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `module_info` expects argument 1 to be of '
                r'type `str` but got type `nil` instead;'):
            await client.query('module_info(nil);')

        res = await client.query('module_info("X");', scope="/t")
        self.assertIsInstance(res.pop('created_at'), int)
        self.assertEqual(res, {
            "name": "X",
            "file": "../modules/x",
            "conf": None,
            "scope": "@collection:stuff",
            "status": "no such file or directory"
        })
        res = await client.query('module_info("X");', scope="/n")
        self.assertIsInstance(res.pop('created_at'), int)
        self.assertEqual(res, {
            "name": "X",
            "file": "../modules/x",
            "conf": None,
            "restarts": 0,
            "scope": "@collection:stuff",
            "status": "no such file or directory",
            "tasks": 0,
        })

    async def test_demo_module(self, client):
        await client.query(r'''
            new_module('DEMO', 'demo/demo', nil, nil);
        ''', scope='/t')

        res = await client.query(r'''
            future({
                module: 'DEMO',
                message: 'Hi ThingsDB!'
            }).then(|msg| {
                `Got this message back: {msg}`
            });
        ''')
        self.assertEqual(res, 'Got this message back: Hi ThingsDB!')

    async def test_requests_module(self, client):
        await client.query(r'''
            new_module('REQUESTS', 'requests/requests', nil, nil);
        ''', scope='/t')

        res = await client.query(r'''
            future({
                module: 'REQUESTS',
                url: 'http://localhost:8080/status'
            }).then(|resp| [str(resp.body).trim(), resp.status_code]);
        ''')
        self.assertEqual(res, ['READY', 200])

    async def _OFF_test_siridb_module(self, client):
        await client.query(r'''
            new_module('SIRIDB', 'siridb/siridb', {
                username: 'iris',
                password: 'siri',
                database: 'dbtest',
                servers: [{
                    host: 'localhost',
                    port: 9000
                }]
            }, nil);
        ''', scope='/t')

        await asyncio.sleep(3)

        res = await client.query(r'''
            future({
                module: 'SIRIDB',
                type: 'QUERY',
                query: 'select * from *'
            });
        ''')
        print(res)


if __name__ == '__main__':
    run_test(TestModules())
