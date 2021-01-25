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
        threshold_full_storage=100,
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

        await client.query(r'''
            new_user('test1');
            set_password("test1", "test1");
            grant('//stuff', 'test1', QUERY);
        ''')

        testcl = await get_client(
            self.node0,
            auth=['test1', 'test1'],
            auto_reconnect=False)

        res = await testcl.query('module_info("X");', scope="//stuff")
        self.assertIsInstance(res.pop('created_at'), int)
        self.assertEqual(res, {
            "name": "X",
            "file": "../modules/x",
            "scope": "@collection:stuff",
            "status": "no such file or directory"
        })

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def test_module_info(self, client):
        res = await client.query(r'''
            new_module("X", "bin", nil, "//stuff");
            new_module("Y", "bin", "conf:123", nil);
        ''', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `modules_info` takes 0 arguments '
                r'but 2 were given;'):
            await client.query('modules_info("X", "x");')

        res = await client.query('modules_info();', scope="/t")
        self.assertEqual(len(res), 2)
        res = await client.query('["X", "Y"].each(|m| del_module(m));')
        self.assertIs(res, None)

    async def test_set_module_conf(self, client):
        res = await client.query(r'''
            new_module("X", "x", nil, nil);
        ''', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `set_module_conf` is undefined in the `@node` '
                r'scope; you might want to query the `@thingsdb` scope\?'):
            await client.query('set_module_conf("X", nil);', scope='/n')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `set_module_conf` takes 2 arguments '
                r'but 1 was given;'):
            await client.query('set_module_conf(nil);', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `set_module_conf` expects argument 1 to be of '
                r'type `str` but got type `nil` instead;'):
            await client.query('set_module_conf(nil, nil);', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'module name must follow the naming rules;'):
            await client.query('set_module_conf("", nil);', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'module `Y` not found'):
            await client.query('set_module_conf("Y", nil);', scope='/t')

        res = await client.query(r'''
            set_module_conf("X", {
                name: "Level1",
                level2: [{
                    name: "Level2",
                    level3: {
                        name: "Level3",
                    }
                }]
            });
            module_info("X");
        ''', scope='/t')

        self.assertEqual(res['conf'], {
            "name": "Level1",
            "level2": [{
                "name": "Level2",
                "level3": {}  # Only 2 deep will be packed
            }]
        })

        res = await client.query(r'''
            set_module_conf("X", 42);
            module_info("X");
        ''')
        self.assertEqual(res['conf'], 42)

        res = await client.query(r'''
            set_module_conf("X", nil);
            module_info("X");
        ''')
        self.assertEqual(res['conf'], None)

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def test_set_module_scope(self, client):
        res = await client.query(r'''
            new_module("X", "x", nil, nil);
        ''', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `set_module_scope` is undefined in the `@node` '
                r'scope; you might want to query the `@thingsdb` scope\?'):
            await client.query('set_module_scope("X", nil);', scope='/n')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `set_module_scope` takes 2 arguments '
                r'but 1 was given;'):
            await client.query('set_module_scope(nil);', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `set_module_scope` expects argument 1 to be of '
                r'type `str` but got type `nil` instead;'):
            await client.query('set_module_scope(nil, nil);', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'module name must follow the naming rules;'):
            await client.query('set_module_scope("", nil);', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'module `Y` not found'):
            await client.query('set_module_scope("Y", nil);', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; the specified collection name is invalid;'):
            await client.query(
                'set_module_scope("X", "///stuff");',
                scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'collection `unknown` not found'):
            await client.query(
                'set_module_scope("X", "//unknown");',
                scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'collection `unknown` not found'):
            await client.query(
                'set_module_scope("X", "//unknown");',
                scope='/t')

        res = await client.query('set_module_scope("X", "/n");', scope='/t')
        self.assertIs(res, None)

        with self.assertRaisesRegex(
                ForbiddenError,
                r'module `X` is restricted to scope `@node`'):
            await client.query(r'''
                future({
                    module: 'X'
                });
            ''', scope='//stuff')

        res = await client.query(
            'set_module_scope("X", "//stuff");',
            scope='/t')
        self.assertIs(res, None)

        with self.assertRaisesRegex(
                OperationError,
                r'module `X` is not running '
                r'\(status: no such file or directory\)'):
            await client.query(r'''
                future({
                    module: 'X'
                });
            ''', scope='//stuff')

        res = await client.query(
            'set_module_scope("X", nil);',
            scope='/t')
        self.assertIs(res, None)

        with self.assertRaisesRegex(
                OperationError,
                r'module `X` is not running '
                r'\(status: no such file or directory\)'):
            await client.query(r'''
                future({
                    module: 'X'
                });
            ''', scope='/t')

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def test_rename_module(self, client):
        res = await client.query(r'''
            new_module("X", "x", nil, nil);
            new_module("Y", "y", nil, nil);
        ''', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `rename_module` is undefined in the `@node` '
                r'scope; you might want to query the `@thingsdb` scope\?'):
            await client.query('rename_module("X", nil);', scope='/n')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `rename_module` takes 2 arguments '
                r'but 1 was given;'):
            await client.query('rename_module(nil);', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `rename_module` expects argument 1 to be of '
                r'type `str` but got type `nil` instead;'):
            await client.query('rename_module(nil, "Y");', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'module name must follow the naming rules;'):
            await client.query('rename_module("", "Y");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'module `Z` not found'):
            await client.query('rename_module("Z", "A");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'module `Y` already exists'):
            await client.query('rename_module("X", "Y");', scope='/t')

        res = await client.query(r'''
            rename_module("X", "Z");
        ''', scope='/t')

        self.assertIs(res, None)

        res = await client.query(r'''
            future({module: 'Z'}).else(||42);
        ''')
        self.assertEqual(res, 42)

        res = await client.query('["Y", "Z"].each(|m| del_module(m));')
        self.assertIs(res, None)

    async def test_del_module(self, client):
        res = await client.query(r'''
            new_module("X", "x", nil, nil);
        ''', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `del_module` is undefined in the `@node` '
                r'scope; you might want to query the `@thingsdb` scope\?'):
            await client.query('del_module("X");', scope='/n')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `del_module` takes 1 argument '
                r'but 0 were given;'):
            await client.query('del_module();', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `del_module` expects argument 1 to be of '
                r'type `str` but got type `int` instead;'):
            await client.query('del_module(1);', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'module name must follow the naming rules;'):
            await client.query('del_module("");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'module `Y` not found'):
            await client.query('del_module("Y");', scope='/t')

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

        with self.assertRaisesRegex(
                LookupError,
                r'module `X` not found'):
            await client.query('del_module("X");', scope='/t')

    async def test_restart_module(self, client):
        res = await client.query(r'''
            new_module("X", "x", nil, nil);
        ''', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `restart_module` is undefined in the `@thingsdb` '
                r'scope; you might want to query a `@node` scope\?'):
            await client.query('restart_module("X");', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `restart_module` takes 1 argument '
                r'but 0 were given;'):
            await client.query('restart_module();', scope='/n')

        with self.assertRaisesRegex(
                TypeError,
                r'function `restart_module` expects argument 1 to be of '
                r'type `str` but got type `int` instead;'):
            await client.query('restart_module(1);', scope='/n')

        with self.assertRaisesRegex(
                ValueError,
                r'module name must follow the naming rules;'):
            await client.query('restart_module("");', scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                r'module `Y` not found'):
            await client.query('restart_module("Y");', scope='/n')

        res = await client.query('restart_module("X");', scope='/n')
        self.assertIs(res, None)

        res = await client.query('del_module("X");', scope='/t')
        self.assertIs(res, None)

    async def test_future_module(self, client):
        res = await client.query(r'''
            new_module("X", "x", nil, nil);
        ''', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `future` requires at least 1 argument '
                r'but 0 were given;'):
            await client.query('future();', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `future` expects argument 1 to be of '
                r'type `thing` or `nil` but got type `str` instead;'):
            await client.query('future("X");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'missing `module` in future request;'):
            await client.query('future({});', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'module `Z` not found'):
            await client.query('future({module: "Z"});', scope='/t')

        with self.assertRaisesRegex(
                OperationError,
                r'module `X` is not running '
                r'\(status: no such file or directory\)'):
            await client.query('future({module: "X"});', scope='/t')

        res = await client.query('future({module: "X"}).else(||nil);')
        self.assertIs(res, None)

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def _OFF_test_demo_module(self, client):
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

    async def _OFF_test_requests_module(self, client):
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
