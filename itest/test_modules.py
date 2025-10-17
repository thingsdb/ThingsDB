#!/usr/bin/env python
import sys
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import ValueError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import ForbiddenError


class TestModules(TestBase):

    title = 'Test modules'

    @default_test_setup(
        num_nodes=2,
        seed=1,
        threshold_full_storage=100,
        python_interpreter=sys.executable)
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node otherwise backups are not possible
        # await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_new_module(self, client):
        client.set_default_scope('//stuff')
        with self.assertRaisesRegex(
                LookupError,
                r'function `new_module` is undefined in the `@collection` '
                r'scope; you might want to query the `@thingsdb` scope\?'):
            await client.query('new_module("X", "x");')

        client.set_default_scope('/t')
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `new_module` requires at least 2 arguments '
                'but 1 was given'):
            await client.query('new_module(1234);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `new_module` takes at most 3 arguments '
                'but 4 were given'):
            await client.query('new_module(1, 2, 3, 4);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `new_module` expects argument 1 to be '
                r'of type `str` but got type `int` instead'):
            await client.query('new_module(1, "x");')

        with self.assertRaisesRegex(
                ValueError,
                r'module name must follow the naming rules'):
            await client.query('new_module("", "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `new_module` expects argument 2 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('new_module("X", 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'source argument must not be an empty string'):
            await client.query('new_module("X", "");')

        with self.assertRaisesRegex(
                ValueError,
                r'source argument contains illegal characters'):
            await client.query('new_module("X", "\n");')

        res = await client.query('new_module("X", "x", nil);')
        self.assertIs(res, None)

        res = await client.query('new_module("Y", "y");')
        self.assertIs(res, None)

        res = await client.query('["X", "Y"].each(|m| del_module(m));')
        self.assertIs(res, None)

    async def test_module_info(self, client):
        res = await client.query(r'''
            new_module("X", "x");
            set_module_scope("X", "//stuff");
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
            "conf": None,
            "scope": "@collection:stuff",
            "status": "module not installed"
        })
        res = await client.query('module_info("X");', scope="/n")
        self.assertIsInstance(res.pop('created_at'), int)
        self.assertEqual(res, {
            "name": "X",
            "conf": None,
            "restarts": 0,
            "scope": "@collection:stuff",
            "status": "module not installed",
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
            "scope": "@collection:stuff",
            "status": "module not installed",
        })

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def test_modules_info(self, client):
        res = await client.query(r'''
            new_module("X", "bin", nil);
            set_module_scope("X", '//stuff');
            new_module("Y", "bin", "conf:123");
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
            new_module("X", "x");
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
                        level4: {
                            name: "Level4",
                        }
                    }
                }]
            });
            module_info("X");
        ''', scope='/t')

        self.assertEqual(res['conf'], {
            "name": "Level1",
            "level2": [{
                "name": "Level2",
                "level3": {
                    "name": "Level3",
                    "level4": {}  # Only 3 levels will be packed
                }
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
            new_module("X", "x");
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
                r'\(status: module not installed\)'):
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
                r'\(status: module not installed\)'):
            await client.query(r'''
                future({
                    module: 'X'
                });
            ''', scope='/t')

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def test_rename_module(self, client):
        res = await client.query(r'''
            new_module("X", "x");
            new_module("Y", "y");
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
            new_module("X", "x");
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
            new_module("X", "x");
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
            new_module("X", "x");
        ''', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `future` requires at least 1 argument '
                r'but 0 were given;'):
            await client.query('future();', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `future` expects argument 1 to be of '
                r'type `thing`, `closure` or `nil` '
                r'but got type `str` instead;'):
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
                r'\(status: module not installed\)'):
            await client.query('future({module: "X"});', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'variable `b` is undefined'):
            await client.query(r'''
                a = 1; c = 3;
                future(|a, b, c| nil);
            ''', scope='/t')

        res = await client.query(r'''
            a = change_id();
            b = 42;
            future(|a, b| {
                c = change_id();
                .arr = [is_int(a), is_int(b), is_int(c)];
            });
        ''', scope='//stuff')

        self.assertEqual(res, [False, True, True])

        res = await client.query('future({module: "X"}).else(||nil);')
        self.assertIs(res, None)

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def test_future_then_module(self, client):
        res = await client.query(r'''
            new_module("X", "x");
        ''', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `then` takes 1 argument but 0 were given;'):
            await client.query('future(nil).then();', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `then` expects argument 1 to be of '
                r'type `closure` but got type `str` instead;'):
            await client.query('future(nil).then("test");', scope='/t')

        with self.assertRaisesRegex(
                OperationError,
                r'only one `then` case is allowed'):
            await client.query('future(nil).then(||nil).then(||nil);')

        res = await client.query('type(future(nil).then(||nil));')
        self.assertEqual(res, 'future')

        res = await client.query(r'''
            a = change_id();
            future(nil, a).then(|_, a| {
                b = change_id();
                .arr = [is_int(a), is_int(b)];
            });
        ''', scope='//stuff')

        self.assertEqual(res, [False, True])

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def test_future_else_module(self, client):
        res = await client.query(r'''
            new_module("X", "x");
        ''', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `else` takes 1 argument but 0 were given;'):
            await client.query('future(nil).else();', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `else` expects argument 1 to be of '
                r'type `closure` but got type `str` instead;'):
            await client.query('future(nil).else("test");', scope='/t')

        with self.assertRaisesRegex(
                OperationError,
                r'only one `else` case is allowed'):
            await client.query('future(nil).else(||nil).else(||nil);')

        res = await client.query('type(future(nil).else(||nil));')
        self.assertEqual(res, 'future')

        res = await client.query(r'''
            a = change_id();
            future({module: "X"}, a).else(|_, a| {
                b = change_id();
                .arr = [is_int(a), is_int(b)];
            });
        ''', scope='//stuff')

        self.assertEqual(res, [False, True])

        res = await client.query('del_module("X");')
        self.assertIs(res, None)

    async def _OFF_test_demo_module(self, client):
        await client.query(r'''
            new_module('DEMO', 'go/demo/demo');
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
        await client.query(r"""//ti
            new_module('requests', 'github.com/thingsdb/module-go-requests');
        """, scope='/t')

        await self.wait_for_module(client, 'requests')

        res = await client.query(r"""//ti
            requests.get('http://localhost:8080/status').then(|resp| {
                resp = resp.load();
                [str(resp.body).trim(), resp.status_code];
            });
        """)
        self.assertEqual(res, ['READY', 200])

        res = await client.query(r"""//ti
            requests.get('http://localhost:8080/status');
        """)
        self.assertEqual(res, [{
            'body': b'READY\n',
            'status': '200 OK',
            'status_code': 200
        }])

    async def test_demo_py_module(self, client):
        await client.query(r"""//ti
             new_module('demo', 'github.com/thingsdb/module-py-demo');
        """, scope='/t')

        await self.wait_for_module(client, 'demo')

        res = await client.query(r"""//ti
            demo.msg("hello!", {
                uppercase: true
            }).then(|reply| reply);
        """)
        self.assertEqual(res, 'HELLO!')

    async def test_deploy(self, client):
        # bug #351
        await client.query(r"""//ti
new_module("test", "test.py");
deploy_module('test',
"from timod import start_module, TiHandler, LookupError, ValueError

class Handler(TiHandler):
    async def on_config(self, req):
        pass

    async def on_request(self, req):
        return 42

if __name__ == '__main__':
    start_module('test', Handler())
");
""", scope='/t')

        await self.wait_for_module(client, 'test')
        res = await client.query(r"""//ti
            set_type('W', {x: ||nil});
            future({
                module: 'test',
                w: {}.wrap('W'),
            }).then(|x| x);
        """, scope='//stuff')
        self.assertEqual(res, 42)


if __name__ == '__main__':
    run_test(TestModules())
