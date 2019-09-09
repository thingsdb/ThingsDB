#!/usr/bin/env python
import asyncio
import pickle
import time
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import BadDataError
from thingsdb.exceptions import IndexError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError
from thingsdb.exceptions import ThingsDBError
from thingsdb import scope
from thingsdb.client.protocol import EX_OVERFLOW
from thingsdb.client.protocol import EX_ZERO_DIV
from thingsdb.client.protocol import EX_MAX_QUOTA
from thingsdb.client.protocol import EX_AUTH_ERROR
from thingsdb.client.protocol import EX_FORBIDDEN
from thingsdb.client.protocol import EX_INDEX_ERROR
from thingsdb.client.protocol import EX_BAD_DATA
from thingsdb.client.protocol import EX_SYNTAX_ERROR
from thingsdb.client.protocol import EX_NODE_ERROR
from thingsdb.client.protocol import EX_ASSERT_ERROR


class TestCollectionFunctions(TestBase):

    title = 'Test collection scope functions'

    @default_test_setup(num_nodes=2, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.run_tests(client)
        # return  # uncomment to skip garbage collection test

        # add another node so away node and gc is forced
        await self.node1.join_until_ready(client)

        # expected no garbage collection
        counters = await client.query('counters();', target=scope.node)
        self.assertEqual(counters['garbage_collected'], 0)

    async def test_unknown(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'function `unknown` is undefined'):
            await client.query('unknown();')

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `unknown`'):
            await client.query('nil.unknown();')

        with self.assertRaisesRegex(
                IndexError,
                'function `new_node` is undefined in the `collection` scope; '
                'You might want to query the `thingsdb` scope?'):
            await client.query('new_node();')

        with self.assertRaisesRegex(
                IndexError,
                'function `counters` is undefined in the `collection` scope; '
                'You might want to query the `node` scope?'):
            await client.query('counters();')

        self.assertIs(await client.query(r'''

        '''), None)

    async def test_add(self, client):
        await client.query(r'.s = set(); .a = {}; .b = {}; .c = {};')
        self.assertEqual(
            await client.query('[.s.add(.a, .b), .s.len()]'), [2, 2])
        self.assertEqual(
            await client.query('[.s.add(.b, .c), .s.len()]'), [1, 3])
        self.assertEqual(
            await client.query(r'[.s.add({}), .s.len()]'), [1, 4])

        with self.assertRaisesRegex(
                BadDataError,
                'cannot add type `nil` to a set'):
            await client.query(r'.s.add(.a, .b, {}, nil);')

    async def test_array(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `array`'):
            await client.query('nil.array();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `array` takes at most 1 argument but 2 were given'):
            await client.query('array(1, 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot convert type `nil` to `array`'):
            await client.query('array(nil);')

        self.assertEqual(await client.query('array();'), [])
        self.assertEqual(await client.query('array( [] );'), [])
        self.assertEqual(await client.query(r'array(set([{}]));'), [{}])

    async def test_assert(self, client):
        with self.assertRaisesRegex(
                AssertionError,
                r'assertion statement `\(1>2\)` has failed'):
            await client.query('assert((1>2));')

        try:
            await client.query('assert((1>2), "my custom message");')
        except AssertionError as e:
            self.assertEqual(str(e), 'my custom message')
            self.assertEqual(e.error_code, EX_ASSERT_ERROR)
        else:
            raise Exception('AssertionError not raised')

        self.assertEqual(await client.query('assert((2>1)); 42;'), 42)

        with self.assertRaisesRegex(
                BadDataError,
                'function `assert` requires at least 1 argument '
                'but 0 were given'):
            await client.query('assert();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `assert` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('assert(true, "x", 1);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `assert` expects argument 2 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('assert(false, nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'assert(false, blob(0));',
                blobs=(pickle.dumps({}), ))

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'assert(false, blob(0));',
                blobs=(pickle.dumps({}), ))

    async def test_assert_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `assert_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('assert_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `assert_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('assert_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'assert_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('assert_err();')
        self.assertEqual(err['error_code'], EX_ASSERT_ERROR)
        self.assertEqual(err['error_msg'], "assertion statement has failed")

        err = await client.query('assert_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_ASSERT_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_auth_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `auth_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('auth_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `auth_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('auth_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'auth_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('auth_err();')
        self.assertEqual(err['error_code'], EX_AUTH_ERROR)
        self.assertEqual(err['error_msg'], "authentication error")

        err = await client.query('auth_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_AUTH_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_bad_data_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `bad_data_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('bad_data_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `bad_data_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('bad_data_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'bad_data_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('bad_data_err();')
        self.assertEqual(err['error_code'], EX_BAD_DATA)
        self.assertEqual(
            err['error_msg'],
            "unable to handle request due to invalid data")

        err = await client.query('bad_data_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_BAD_DATA)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_blob(self, client):
        self.assertIs(await client.query(
            '.objects = [blob(0), blob(1)]; nil;',
            blobs=(pickle.dumps({}), pickle.dumps([]))), None)

        d, a = await client.query('.objects;')
        self.assertTrue(isinstance(pickle.loads(d), dict))
        self.assertTrue(isinstance(pickle.loads(a), list))

        with self.assertRaisesRegex(
                BadDataError,
                'function `blob` takes 1 argument but 0 were given'):
            await client.query('blob();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `blob` expects argument 1 to be of '
                'type `int` but got type `nil` instead'):
            await client.query('blob(nil);')

        with self.assertRaisesRegex(
                IndexError, 'blob index out of range'):
            await client.query('blob(0);')

    async def test_bool(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `bool`'):
            await client.query('nil.bool();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `bool` takes at most 1 argument but 2 were given'):
            await client.query('bool(1, 2);')

        self.assertFalse(await client.query('bool();'))
        self.assertFalse(await client.query('bool(false);'))
        self.assertFalse(await client.query('bool(nil);'))
        self.assertFalse(await client.query('bool(0);'))
        self.assertFalse(await client.query('bool(-0);'))
        self.assertFalse(await client.query('bool(0.0);'))
        self.assertFalse(await client.query('bool(-0.0);'))
        self.assertFalse(await client.query('bool("");'))
        self.assertFalse(await client.query('bool([]);'))
        self.assertFalse(await client.query('bool(set());'))
        self.assertFalse(await client.query(r'bool({});'))

        self.assertTrue(await client.query('bool(true);'))
        self.assertTrue(await client.query('bool(!nil);'))
        self.assertTrue(await client.query('bool(1);'))
        self.assertTrue(await client.query('bool(-1);'))
        self.assertTrue(await client.query('bool(1.1);'))
        self.assertTrue(await client.query('bool(-1.1);'))
        self.assertTrue(await client.query('bool("abc");'))
        self.assertTrue(await client.query('bool([0]);'))
        self.assertTrue(await client.query(r'bool(set([{}]));'))
        self.assertTrue(await client.query(r'bool({answer: 42});'))
        self.assertTrue(await client.query('bool(//);'))
        self.assertTrue(await client.query('bool(||nil);'))

    async def test_call(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `call`'):
            await client.query('nil.call();')

        with self.assertRaisesRegex(
                IndexError,
                'function `call` is undefined'):
            await client.query('call();')

        with self.assertRaisesRegex(
                BadDataError,
                'this closure takes 0 arguments but 1 was given'):
            await client.query('(||nil).call(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                r'stored closures with side effects must be wrapped '
                r'using `wse\(...\)`'):
            await client.query(r'''
                .test = |x| .x = x;
                .test.call(42);
            ''')

        self.assertEqual(await client.query('(|y|.y = y).call(42);'), 42)
        self.assertEqual(await client.query('wse(.test.call(42));'), 42)

        self.assertEqual(await client.query('.x'), 42)
        self.assertEqual(await client.query('.y'), 42)

    async def test_contains(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `contains`'):
            await client.query('nil.contains("world!");')

        with self.assertRaisesRegex(
                BadDataError,
                'function `contains` takes 1 argument but 2 were given'):
            await client.query('"Hi World!".contains("x", 2)')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `contains` expects argument 1 to be of '
                r'type `raw` but got type `int` instead'):
            await client.query('"Hi World!".contains(1);')

        self.assertTrue(await client.query(r'''
            "Hi World!".contains("")
        '''))
        self.assertTrue(await client.query(r'''
            "Hi World!".contains("World")
        '''))
        self.assertTrue(await client.query(r'''
            "Hi World!".contains("H")
        '''))
        self.assertTrue(await client.query(r'''
            "Hi World!".contains("!")
        '''))
        self.assertFalse(await client.query(r'''
            "Hi World!".contains("world")
        '''))

    async def test_deep(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `deep`'):
            await client.query('nil.deep();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `deep` takes 0 arguments but 1 was given'):
            await client.query('deep(nil);')

        self.assertEqual(await client.query('deep();'), 1)

        self.assertEqual(await client.query(r'''
            ( ||return(nil, 0) ).call();
            deep();
        '''), 0)

        self.assertEqual(await client.query(r'''
            ( ||return(nil, 2) ).call();
            deep();
        '''), 2)

    async def test_del(self, client):
        await client.query(r'.greet = "Hello world";')

        with self.assertRaisesRegex(
                IndexError,
                'type `raw` has no function `del`'):
            await client.query('.greet.del("x");')

        with self.assertRaisesRegex(
                BadDataError,
                'function `del` takes 1 argument '
                'but 0 were given'):
            await client.query('.del();')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change type `thing` while the value is being used'):
            await client.query('.map( ||.del("greet") );')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `del` expects argument 1 to be of type `raw` '
                r'but got type `nil` instead'):
            await client.query('.del(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                r'property name must follow the naming rules'):
            await client.query('.del("");')

        with self.assertRaisesRegex(
                IndexError,
                r'thing `#\d+` has no property `x`'):
            await client.query('.del("x");')

        self.assertIs(await client.query(r'.del("greet");'), None)
        self.assertFalse(await client.query(r'.has("greet");'))

    async def test_endswith(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `endswith`'):
            await client.query('nil.endswith("world!");')

        with self.assertRaisesRegex(
                BadDataError,
                'function `endswith` takes 1 argument but 2 were given'):
            await client.query('"Hi World!".endswith("x", 2)')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `endswith` expects argument 1 to be of '
                r'type `raw` but got type `int` instead'):
            await client.query('"Hi World!".endswith(1);')

        self.assertTrue(await client.query('"Hi World!".endswith("")'))
        self.assertFalse(await client.query('"".endswith("!")'))
        self.assertTrue(await client.query('"Hi World!".endswith("World!")'))
        self.assertFalse(await client.query('"Hi World!".endswith("world!")'))

    async def test_extend(self, client):
        await client.query('.list = [];')
        self.assertEqual(await client.query('.list.extend(["a"])'), 1)
        self.assertEqual(await client.query('.list.extend(["b", "c"])'), 3)
        self.assertEqual(await client.query('.list;'), ['a', 'b', 'c'])

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `extend`'):
            await client.query('nil.extend();')

        with self.assertRaisesRegex(
                IndexError,
                'function `extend` is undefined'):
            await client.query('extend();')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `extend`'):
            await client.query('.a = [.list]; .a[0].extend(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `extend` takes 1 argument '
                'but 0 were given'):
            await client.query('.list.extend();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `extend` expects argument 1 to be of '
                'type `array` but got type `nil` instead'):
            await client.query('.list.extend(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.extend([4]));')

    async def test_filter(self, client):
        await client.query(r'''
            .iris = {
                name: 'Iris',
                age: 6,
                likes: ['k3', 'swimming', 'red', 6],
            };
            .cato = {
                name: 'Cato',
                age: 5,
            };
            .girls = set([.iris, .cato]);
        ''')
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `filter`'):
            await client.query('nil.filter(||true);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `filter` takes 1 argument but 0 were given'):
            await client.query('.filter();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `filter` expects argument 1 to be a `closure` '
                'but got type `nil` instead'):
            await client.query('.filter(nil);')

        self.assertEqual(
            (await client.query('.iris.filter(|k|(k == "name"));')),
            {'name': 'Iris'})

        self.assertEqual(
            (await client.query('.iris.filter(|_k, v|(v == 6));')),
            {'age': 6})

        self.assertEqual(
            (await client.query('.iris.likes.filter(|v|isstr(v));')),
            ['k3', 'swimming', 'red'])

        self.assertEqual(
            (await client.query('.iris.likes.filter(|_v, i|(i > 1));')),
            ['red', 6])

        self.assertEqual(
            (await client.query('.girls.filter(|v|(v.age == 6))'))
            ['$'][0]['age'],
            6)

        self.assertEqual(
            (await client.query(
                '.girls.filter(|_v, i|(i == .cato.id()))'))['$'][0]['age'],
            5)

        self.assertEqual(await client.query('.iris.filter(||nil);'), {})
        self.assertEqual(await client.query('.iris.likes.filter(||nil);'), [])
        self.assertEqual(await client.query(r'{}.filter(||true)'), {})
        self.assertEqual(await client.query(r'[].filter(||true)'), [])
        self.assertEqual(await client.query(r'set().filter(||1)'), {'$': []})

    async def test_find(self, client):
        await client.query(r'''
            .x = [42, "gallaxy"];
            .iris = {
                name: 'Iris',
                age: 6,
                likes: ['k3', 'swimming', 'red', 6],
            };
            .cato = {
                name: 'Cato',
                age: 5,
            };
            .g = set([.iris, .cato]);
            ''')

        with self.assertRaisesRegex(
                IndexError,
                'type `thing` has no function `find`'):
            await client.query('.find(||true);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `find` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.x.find();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `find` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('.x.find(||true, 2, 3);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `find` expects argument 1 to be a `closure` '
                'but got type `int` instead'):
            await client.query('.x.find(0);')

        iris, cato = await client.query('[.iris, .cato];')

        self.assertIs(await client.query('[].find(||true);'), None)
        self.assertIs(await client.query('[].find(||false);'), None)
        self.assertEqual(await client.query('.x.find(||true);'), 42)
        self.assertEqual(await client.query('.x.find(|v|(v==42));'), 42)
        self.assertEqual(await client.query('.x.find(|_,i|(i>=0));'), 42)
        self.assertEqual(await client.query(
            '.x.find(|_,i|(i>=1));'),
            "gallaxy")
        self.assertIs(await client.query('.x.find(||nil);'), None)
        self.assertEqual(await client.query('.x.find(||nil, 42);'), 42)
        self.assertEqual(await client.query('.g.find(|t|(t.age==5));'), cato)
        self.assertEqual(await client.query(
            '.g.find(|_,i|(i==.iris.id()));'), iris)
        self.assertIs(await client.query('.g.find(||nil);'), None)

    async def test_findindex(self, client):
        await client.query(r'.x = [42, ""];')

        with self.assertRaisesRegex(
                IndexError,
                'type `thing` has no function `findindex`'):
            await client.query('.findindex(||true);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `findindex` takes 1 argument but 2 were given'):
            await client.query('.x.findindex(0, 1);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `findindex` expects argument 1 to be a `closure` '
                'but got type `bool` instead'):
            await client.query('.x.findindex(true);')

        self.assertIs(await client.query('[].findindex(||true);'), None)
        self.assertIs(await client.query('[].findindex(||false);'), None)
        self.assertEqual(await client.query('.x.findindex(||true);'), 0)
        self.assertIs(await client.query('.x.findindex(||false);'), None)
        self.assertEqual(await client.query('.x.findindex(|_,i|(i>0));'), 1)
        self.assertEqual(await client.query('.x.findindex(|v|(v==42));'), 0)
        self.assertEqual(await client.query('.x.findindex(|v|isstr(v));'), 1)

    async def test_float(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `float`'):
            await client.query('nil.float();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `float` takes at most 1 argument but 2 were given'):
            await client.query('float(1, 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot convert type `closure` to `float`'):
            await client.query('float(||nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot convert type `nil` to `float`'):
            await client.query('float(nil);')

        self.assertEqual(
            await client.query('int("0x7fffffffffffffff");'),
            0x7fffffffffffffff)
        self.assertEqual(
            await client.query('int("-0x8000000000000000");'),
            -0x8000000000000000)
        self.assertEqual(await client.query('float();'), 0.0)
        self.assertEqual(await client.query('float(3.14);'), 3.14)
        self.assertEqual(await client.query('float(42);'), 42.0)
        self.assertEqual(await client.query('float(true);'), 1.0)
        self.assertEqual(await client.query('float(false);'), 0.0)
        self.assertEqual(await client.query('int(true);'), 1)
        self.assertEqual(await client.query('int(false);'), 0)
        self.assertEqual(await client.query('float("3.14");'), 3.14)
        self.assertEqual(await client.query('float("-0.314e+1");'), -3.14)
        self.assertEqual(await client.query('float("");'), 0.0)

    async def test_forbidden_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `forbidden_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('forbidden_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `forbidden_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('forbidden_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'forbidden_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('forbidden_err();')
        self.assertEqual(err['error_code'], EX_FORBIDDEN)
        self.assertEqual(err['error_msg'], "forbidden (access denied)")

        err = await client.query('forbidden_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_FORBIDDEN)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_get(self, client):
        await client.query(r'.iris = {name: "Iris"};')

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `get`'):
            await client.query('nil.get();')

        with self.assertRaisesRegex(
                IndexError,
                'function `get` is undefined'):
            await client.query('get();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `get` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.get();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `get` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('.get("x", nil, nil);')

        self.assertIs(await client.query('.iris.get("x");'), None)
        self.assertEqual(await client.query('.iris.get("x", "?");'), '?')
        self.assertEqual(await client.query('.iris.get("name");'), 'Iris')

    async def test_has_set(self, client):
        await client.query(r'''
            .iris = {name: "Iris"};
            .cato = {name: "Cato"};
            .s = set([.iris]);
        ''')

        with self.assertRaisesRegex(
                IndexError,
                'function `has` is undefined'):
            await client.query('has(.iris);')

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `has`'):
            await client.query('nil.has(.iris);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `has` takes 1 argument but 0 were given'):
            await client.query('.s.has();')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `has` expects argument 1 to be of '
                r'type `thing` but got type `int` instead'):
            await client.query('.s.has(0);')

        self.assertTrue(await client.query('.s.has(.iris);'))
        self.assertFalse(await client.query('.s.has(.cato);'))

    async def test_has_thing(self, client):
        await client.query(r'.x = 0.0;')

        with self.assertRaisesRegex(
                IndexError,
                'type `float` has no function `has`'):
            await client.query('.x.has("x");')

        with self.assertRaisesRegex(
                BadDataError,
                'function `has` takes 1 argument but 0 were given'):
            await client.query('.has();')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `has` expects argument 1 to be of '
                r'type `raw` but got type `int` instead'):
            await client.query('.has(.id());')

        self.assertTrue(await client.query('.has("x");'))
        self.assertFalse(await client.query('.has("y");'))

    async def test_id(self, client):
        o = await client.query(r'.o = {};')
        self.assertTrue(isinstance(o['#'], int))
        self.assertGreater(o['#'], 0)
        self.assertEqual(await client.query(r'.o.id();'), o['#'])
        self.assertIs(await client.query(r'{}.id();'), None)

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `id`'):
            await client.query('nil.id();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `id` takes 0 arguments but 1 was given'):
            await client.query('.id(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `id` takes 0 arguments but 2 were given'):
            await client.query('.id(nil, nil);')

    async def test_indexof(self, client):
        await client.query(
            r'.x = [42, "thingsdb", thing(.id()), 42, false, nil];')

        with self.assertRaisesRegex(
                IndexError,
                'type `thing` has no function `indexof`'):
            await client.query('.indexof("x");')

        with self.assertRaisesRegex(
                BadDataError,
                'function `indexof` takes 1 argument but 0 were given'):
            await client.query('.x.indexof();')

        self.assertEqual(await client.query('.x.indexof(42);'), 0)
        self.assertEqual(await client.query('.x.indexof("thingsdb");'), 1)
        self.assertEqual(await client.query('.x.indexof(thing(.id()));'), 2)
        self.assertEqual(await client.query('.x.indexof(false);'), 4)
        self.assertEqual(await client.query('.x.indexof(nil);'), 5)
        self.assertIs(await client.query('.x.indexof(42.1);'), None)
        self.assertIs(await client.query('.x.indexof("ThingsDb");'), None)
        self.assertIs(await client.query('.x.indexof(true);'), None)
        self.assertIs(await client.query(r'.x.indexof({});'), None)

        # cleanup garbage, the reference to the collection
        await client.query(r'''.x.splice(2, 1);''')

    async def test_index_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `index_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('index_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `index_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('index_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'index_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('index_err();')
        self.assertEqual(err['error_code'], EX_INDEX_ERROR)
        self.assertEqual(err['error_msg'], "requested resource not found")

        err = await client.query('index_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_INDEX_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_int(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `int`'):
            await client.query('nil.int();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `int` takes at most 1 argument but 2 were given'):
            await client.query('int(1, 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot convert type `regex` to `int`'):
            await client.query('int(/.*/);')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot convert type `nil` to `int`'):
            await client.query('int(nil);')

        with self.assertRaisesRegex(
                OverflowError,
                'integer overflow'):
            await client.query('int("0x8000000000000000");')

        with self.assertRaisesRegex(
                OverflowError,
                'integer overflow'):
            await client.query('int("-0x8000000000000001");')

        with self.assertRaisesRegex(
                OverflowError,
                'integer overflow'):
            await client.query('int(9223372036854775808.314);')

        self.assertEqual(
            await client.query('int("0x7fffffffffffffff");'),
            0x7fffffffffffffff)
        self.assertEqual(
            await client.query('int("-0x8000000000000000");'),
            -0x8000000000000000)
        self.assertEqual(await client.query('int();'), 0)
        self.assertEqual(await client.query('int(3.14);'), 3)
        self.assertEqual(await client.query('int(-3.14);'), -3)
        self.assertEqual(await client.query('int(42.9);'), 42)
        self.assertEqual(await client.query('int(-42.9);'), -42)
        self.assertEqual(await client.query('int(true);'), 1)
        self.assertEqual(await client.query('int(false);'), 0)
        self.assertEqual(await client.query('int("3.14");'), 3)
        self.assertEqual(await client.query('int("-3.14");'), -3)

    async def test_isarray(self, client):
        await client.query('.x = [[0, 1], nil]; .y = .x[0];')
        with self.assertRaisesRegex(
                BadDataError,
                'function `isarray` takes 1 argument but 0 were given'):
            await client.query('isarray();')

        self.assertTrue(await client.query('isarray([]);'))
        self.assertTrue(await client.query('isarray(.x);'))
        self.assertTrue(await client.query('isarray(.y);'))
        self.assertTrue(await client.query('isarray(.x[0]);'))
        self.assertFalse(await client.query('isarray(0);'))
        self.assertFalse(await client.query('isarray("test");'))
        self.assertFalse(await client.query(r'isarray({});'))
        self.assertFalse(await client.query('isarray(.x[1]);'))
        self.assertFalse(await client.query('isarray(set());'))

    async def test_isascii(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isascii` takes 1 argument but 0 were given'):
            await client.query('isascii();')

        self.assertTrue(await client.query('isascii( "pi" ); '))
        self.assertTrue(await client.query('isascii( "" ); '))
        self.assertFalse(await client.query('isascii( "ԉ" ); '))
        self.assertFalse(await client.query('isascii([]);'))
        self.assertFalse(await client.query('isascii(nil);'))
        self.assertFalse(await client.query(
                'isascii(blob(0));',
                blobs=(pickle.dumps('binary'), )))

    async def test_isbool(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isbool` takes 1 argument but 3 were given'):
            await client.query('isbool(1, 2, 3);')

        self.assertTrue(await client.query('isbool( true ); '))
        self.assertTrue(await client.query('isbool( false ); '))
        self.assertTrue(await client.query('isbool( isint(.id()) ); '))
        self.assertFalse(await client.query('isbool( "ԉ" ); '))
        self.assertFalse(await client.query('isbool([]);'))
        self.assertFalse(await client.query('isbool(nil);'))

    async def test_iserr(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `iserr` takes 1 argument but 2 were given'):
            await client.query('iserr(1, 2);')

        self.assertTrue(await client.query('iserr( err() ); '))
        self.assertTrue(await client.query('iserr( zero_div_err() ); '))
        self.assertTrue(await client.query('iserr( try ((1/0)) ); '))
        self.assertFalse(await client.query('iserr( "ԉ" ); '))
        self.assertFalse(await client.query('iserr([]);'))
        self.assertFalse(await client.query('iserr(nil);'))

    async def test_isfloat(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isfloat` takes 1 argument but 0 were given'):
            await client.query('isfloat();')

        self.assertTrue(await client.query('isfloat( 0.0 ); '))
        self.assertTrue(await client.query('isfloat( -0.0 ); '))
        self.assertTrue(await client.query('isfloat( inf ); '))
        self.assertTrue(await client.query('isfloat( -inf ); '))
        self.assertTrue(await client.query('isfloat( nan ); '))
        self.assertFalse(await client.query('isfloat( "ԉ" ); '))
        self.assertFalse(await client.query('isfloat( 42 );'))
        self.assertFalse(await client.query('isfloat( nil );'))

    async def test_isinf(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isinf` takes 1 argument but 0 were given'):
            await client.query('isinf();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `isinf` expects argument 1 to be of '
                'type `float` but got type `nil` instead'):
            await client.query('isinf(nil);')

        self.assertTrue(await client.query('isinf( inf ); '))
        self.assertTrue(await client.query('isinf( -inf ); '))
        self.assertFalse(await client.query('isinf( 0.0 ); '))
        self.assertFalse(await client.query('isinf( nan ); '))
        self.assertFalse(await client.query('isinf( 42 ); '))
        self.assertFalse(await client.query('isinf( true ); '))

    async def test_isint(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isint` takes 1 argument but 0 were given'):
            await client.query('isint();')

        self.assertTrue(await client.query('isint( 0 ); '))
        self.assertTrue(await client.query('isint( -0 ); '))
        self.assertTrue(await client.query('isint( 42 );'))
        self.assertFalse(await client.query('isint( 0.0 ); '))
        self.assertFalse(await client.query('isint( -0.0 ); '))
        self.assertFalse(await client.query('isint( inf ); '))
        self.assertFalse(await client.query('isint( -inf ); '))
        self.assertFalse(await client.query('isint( nan ); '))
        self.assertFalse(await client.query('isint( "ԉ" ); '))
        self.assertFalse(await client.query('isint( nil );'))
        self.assertFalse(await client.query('isint( set() );'))

    async def test_islist(self, client):
        await client.query('.x = [[0, 1], nil]; .y = .x[0];')
        with self.assertRaisesRegex(
                BadDataError,
                'function `islist` takes 1 argument but 0 were given'):
            await client.query('islist();')

        self.assertTrue(await client.query('islist([]);'))
        self.assertTrue(await client.query('islist(.x);'))
        self.assertTrue(await client.query('islist(.y);'))
        self.assertFalse(await client.query('islist(.x[0]);'))
        self.assertFalse(await client.query('islist(0);'))
        self.assertFalse(await client.query('islist("test");'))
        self.assertFalse(await client.query(r'islist({});'))
        self.assertFalse(await client.query('islist(.x[1]);'))
        self.assertFalse(await client.query('islist(set());'))

    async def test_isnan(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isnan` takes 1 argument but 0 were given'):
            await client.query('isnan();')

        self.assertFalse(await client.query('isnan( inf ); '))
        self.assertFalse(await client.query('isnan( -inf ); '))
        self.assertFalse(await client.query('isnan( 3.14 ); '))
        self.assertFalse(await client.query('isnan( 42 ); '))
        self.assertFalse(await client.query('isnan( true ); '))
        self.assertTrue(await client.query('isnan( nan ); '))
        self.assertTrue(await client.query('isnan( [] ); '))
        self.assertTrue(await client.query(r'isnan( {} ); '))
        self.assertTrue(await client.query('isnan( "3" ); '))
        self.assertTrue(await client.query('isnan( set() ); '))

    async def test_isnil(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isnil` takes 1 argument but 0 were given'):
            await client.query('isnil();')

        self.assertTrue(await client.query('isnil( nil ); '))
        self.assertFalse(await client.query('isnil( 0 ); '))

    async def test_israw(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `israw` takes 1 argument but 0 were given'):
            await client.query('israw();')

        self.assertTrue(await client.query('israw( "pi" ); '))
        self.assertTrue(await client.query('israw( "" ); '))
        self.assertTrue(await client.query('israw( "ԉ" ); '))
        self.assertFalse(await client.query('israw([]);'))
        self.assertFalse(await client.query('israw(nil);'))
        self.assertTrue(await client.query(
                'israw(blob(0));',
                blobs=(pickle.dumps('binary'), )))

    async def test_isset(self, client):
        await client.query(r'.sa = set(); .sb = set([ {} ]);')
        with self.assertRaisesRegex(
                BadDataError,
                'function `isset` takes 1 argument but 0 were given'):
            await client.query('isset();')

        self.assertFalse(await client.query(r'isset({});'))
        self.assertFalse(await client.query('isset([]);'))
        self.assertTrue(await client.query('isset(set());'))
        self.assertTrue(await client.query('isset(.sa);'))
        self.assertTrue(await client.query('isset(.sb);'))

    async def test_isstr(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isstr` takes 1 argument but 0 were given'):
            await client.query('isstr();')

        self.assertTrue(await client.query('isstr( "pi" ); '))
        self.assertTrue(await client.query('isstr( "" ); '))
        self.assertTrue(await client.query('isstr( "ԉ" ); '))
        self.assertFalse(await client.query('isstr([]);'))
        self.assertFalse(await client.query('isstr(nil);'))
        self.assertFalse(await client.query('isstr(123);'))
        self.assertFalse(await client.query(
                'isstr(blob(0));',
                blobs=(pickle.dumps('binary'), )))

    async def test_isthing(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isthing` takes 1 argument but 0 were given'):
            await client.query('isthing();')

        id = await client.query('.id();')

        self.assertTrue(await client.query(f'isthing( #{id} ); '))
        self.assertTrue(await client.query(r'isthing( {} ); '))
        self.assertTrue(await client.query(r'isthing( thing(.id()) ); '))
        self.assertFalse(await client.query('isthing( [] ); '))
        self.assertFalse(await client.query('isthing( set() ); '))

    async def test_istuple(self, client):
        await client.query('.x = [[0, 1], nil]; .y = .x[0];')
        with self.assertRaisesRegex(
                BadDataError,
                'function `istuple` takes 1 argument but 0 were given'):
            await client.query('istuple();')

        self.assertFalse(await client.query('istuple([]);'))
        self.assertFalse(await client.query('istuple(.x);'))
        self.assertFalse(await client.query('istuple(.y);'))
        self.assertTrue(await client.query('istuple(.x[0]);'))
        self.assertFalse(await client.query('istuple(0);'))
        self.assertFalse(await client.query('istuple("test");'))
        self.assertFalse(await client.query(r'istuple({});'))
        self.assertFalse(await client.query('istuple(.x[1]);'))

    async def test_isutf8(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `isutf8` takes 1 argument but 0 were given'):
            await client.query('isutf8();')

        self.assertTrue(await client.query('isutf8( "pi" ); '))
        self.assertTrue(await client.query('isutf8( "" ); '))
        self.assertTrue(await client.query('isutf8( "ԉ" ); '))
        self.assertFalse(await client.query('isutf8([]);'))
        self.assertFalse(await client.query('isutf8(nil);'))
        self.assertFalse(await client.query('isutf8(123);'))
        self.assertFalse(await client.query(
                'isutf8(blob(0));',
                blobs=(pickle.dumps('binary'), )))

    async def test_keys(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `keys`'):
            await client.query('nil.keys();')

        with self.assertRaisesRegex(
                IndexError,
                'function `keys` is undefined'):
            await client.query('keys();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `keys` takes 0 arguments but 1 was given'):
            await client.query('.keys(nil);')

        self.assertEqual(await client.query(r'{}.keys();'), [])
        self.assertEqual(await client.query(r'{a:1}.keys();'), ['a'])
        self.assertEqual(await client.query(r'{a:1, b:2}.keys();'), ['a', 'b'])

    async def test_len(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `len`'):
            await client.query('nil.len();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `len` takes 0 arguments but 1 was given'):
            await client.query('.len(nil);')

        self.assertEqual(await client.query(r'{}.len();'), 0)
        self.assertEqual(await client.query(r'"".len();'), 0)
        self.assertEqual(await client.query(r'[].len();'), 0)
        self.assertEqual(await client.query(r'[[]][0].len();'), 0)
        self.assertEqual(await client.query(r'{x:0, y:1}.len();'), 2)
        self.assertEqual(await client.query(r'"xy".len();'), 2)
        self.assertEqual(await client.query(r'["x", "y"].len();'), 2)
        self.assertEqual(await client.query(r'[["x", "y"]][0].len();'), 2)
        self.assertEqual(await client.query(r'set([{}, {}]).len();'), 2)

    async def test_lower(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `lower`'):
            await client.query('nil.lower();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `lower` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".lower(nil);')

        self.assertEqual(await client.query('"".lower();'), "")
        self.assertEqual(await client.query('"l".lower();'), "l")
        self.assertEqual(await client.query('"HI !!".lower();'), "hi !!")

    async def test_map(self, client):
        await client.query(r'''
            .iris = {
                name: 'Iris',
                age: 6,
                likes: ['k3', 'swimming', 'red', 6],
            };
            .cato = {
                name: 'Cato',
                age: 5,
            };
            .girls = set([.iris, .cato]);
        ''')
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `map`'):
            await client.query('nil.map(||nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `map` takes 1 argument but 0 were given'):
            await client.query('.map();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `map` expects argument 1 to be a `closure` '
                'but got type `nil` instead'):
            await client.query('.map(nil);')

        self.assertEqual(await client.query(r'[].map(||nil)'), [])
        self.assertEqual(await client.query(r'{}.map(||nil)'), [])
        self.assertEqual(await client.query(r'set().map(||nil)'), [])

        self.assertEqual(
            set(await client.query('.iris.map(|k|k.upper());')),
            set({'NAME', 'AGE', 'LIKES'}))

        self.assertEqual(
            set(await client.query(
                '.iris.map(|_, v| (isarray(v)) ? true:v);')),
            set({'Iris', 6, True}))

        self.assertEqual(
            await client.query('.iris.likes.map(|v, i|[v, i])'),
            [['k3', 0], ['swimming', 1], ['red', 2], [6, 3]])

        self.assertEqual(
            set(await client.query('.girls.map(|g| g.name);')),
            set(['Iris', 'Cato'])
        )

        iris_id, cato_id = await client.query('[.iris.id(), .cato.id()];')

        self.assertEqual(
            set(await client.query('.girls.map(|_, id| id);')),
            set([iris_id, cato_id])
        )

    async def test_max_quota_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `max_quota_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('max_quota_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `max_quota_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('max_quota_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'max_quota_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('max_quota_err();')
        self.assertEqual(err['error_code'], EX_MAX_QUOTA)
        self.assertEqual(err['error_msg'], "max quota is reached")

        err = await client.query('max_quota_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_MAX_QUOTA)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_node_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `node_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('node_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `node_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('node_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'node_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('node_err();')
        self.assertEqual(err['error_code'], EX_NODE_ERROR)
        self.assertEqual(
            err['error_msg'],
            "node is temporary unable to handle the request")

        err = await client.query('node_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_NODE_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_now(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `now` takes 0 arguments but 2 were given'):
            await client.query('now(0, 1);')

        now = await client.query('now();')
        diff = time.time() - now
        self.assertGreater(diff, 0.0)
        self.assertLess(diff, 0.02)

    async def test_overflow_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `overflow_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('overflow_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `overflow_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('overflow_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'overflow_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('overflow_err();')
        self.assertEqual(err['error_code'], EX_OVERFLOW)
        self.assertEqual(err['error_msg'], "integer overflow")

        err = await client.query('overflow_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_OVERFLOW)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_pop(self, client):
        await client.query('.list = [1, 2, 3];')
        self.assertEqual(await client.query('.list.pop()'), 3)
        self.assertEqual(await client.query('.list.pop()'), 2)
        self.assertEqual(await client.query('.list;'), [1])
        self.assertIs(await client.query('[nil].pop()'), None)

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `pop`'):
            await client.query('nil.pop();')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `pop`'):
            await client.query('.a = [.list]; .a[0].pop();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `pop` takes 0 arguments but 1 was given'):
            await client.query('.list.pop(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.pop());')

        with self.assertRaisesRegex(
                IndexError,
                'pop from empty array'):
            await client.query('[].pop();')

    async def test_push(self, client):
        await client.query('.list = [];')
        self.assertEqual(await client.query('.list.push("a")'), 1)
        self.assertEqual(await client.query('.list.push("b", "c")'), 3)
        self.assertEqual(await client.query('.list;'), ['a', 'b', 'c'])

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `push`'):
            await client.query('nil.push();')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `push`'):
            await client.query('.a = [.list]; .a[0].push(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `push` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.list.push();')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.push(4));')

    async def test_raise(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `raise`'):
            await client.query('nil.raise();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `raise` takes at most 1 argument but 2 were given'):
            await client.query('raise(err(), 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `raise` expects argument 1 to be of type `error` '
                'but got type `nil` instead'):
            await client.query('raise(nil);')

        with self.assertRaisesRegex(
                ThingsDBError,
                'error:-100'):
            await client.query('raise();')

        with self.assertRaisesRegex(
                ThingsDBError,
                'my custom error'):
            await client.query('raise(err(-100, "my custom error"));')

    async def test_refs(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `raw` has no function `refs`'):
            await client.query('"".refs();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `refs` takes 1 argument but 0 were given'):
            await client.query('refs();')

        self.assertGreater(await client.query('refs(nil);'), 1)
        self.assertGreater(await client.query('refs(true);'), 1)
        self.assertGreater(await client.query('refs(false);'), 1)
        self.assertEqual(await client.query('refs(thing(.id()));'), 2)
        self.assertEqual(await client.query('refs("Test RefCount")'), 2)
        self.assertEqual(await client.query('x = "Test RefCount"; refs(x)'), 3)

    async def test_remove_list(self, client):
        await client.query('.list = [1, 2, 3];')
        self.assertEqual(await client.query('.list.remove(|x|(x>1));'), 2)
        self.assertEqual(await client.query('.list.remove(|x|(x>1));'), 3)
        self.assertEqual(await client.query('.list;'), [1])
        self.assertIs(await client.query('.list.remove(||false);'), None)
        self.assertEqual(await client.query('.list.remove(||false, "");'), '')
        self.assertIs(await client.query('[].remove(||true);'), None)
        self.assertEqual(await client.query('["pi"].remove(||true);'), 'pi')

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `remove`'):
            await client.query('nil.remove(||true);')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `remove`'):
            await client.query('.a = [.list]; .a[0].remove(||true);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `remove` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.list.remove();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `remove` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('.list.remove(||true, 2, 3);')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.remove(||true));')

        with self.assertRaisesRegex(
                BadDataError,
                'function `remove` expects argument 1 to be a `closure` '
                'but got type `nil` instead'):
            await client.query('.list.remove(nil);')

    async def test_remove_set(self, client):
        await client.query(r'''
        .t = {name: 'ThingsDB'};
        .s = set([.t, {name: 'Iris'}, {name: 'Cato'}]);
        ''')

        removed = await client.query('.s.remove(|T| (T == .t));')

        self.assertEqual(len(removed), 1)
        self.assertEqual(removed[0]['name'], 'ThingsDB')
        self.assertEqual(await client.query('.s.remove(||nil);'), [])

        removed = await client.query('.s.remove(||true);')

        self.assertEqual(len(removed), 2)

        await client.query(r'''
            .i = {name: 'Iris'};
            .c = {name: 'Cato'};
            .s.add(.t, .i, .c);
        ''')

        removed = await client.query('.s.remove(|_, id| (id == .t.id()));')
        self.assertEqual(len(removed), 1)
        self.assertEqual(removed[0]['name'], 'ThingsDB')

        removed = await client.query('.s.remove(.t, .i, .c);')

        self.assertEqual(len(removed), 2)

        await client.query(r'.s.add(.t, .i, .c);')

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `remove`'):
            await client.query('nil.remove(||true);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `remove` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.s.remove();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `remove` takes at most 1 argument '
                'when using a `closure` but 2 were given'):
            await client.query('.s.remove(||true, nil);')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change type `set` while the value is being used'):
            await client.query('.s.map(||.s.remove(||true));')

        with self.assertRaisesRegex(
                BadDataError,
                'function `remove` expects argument 1 to be a `closure` '
                'or type `thing` but got type `nil` instead'):
            await client.query('.s.remove(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `remove` expects argument 2 to be of type `thing` '
                'but got type `nil` instead'):
            await client.query('.s.remove(.t, nil);')

        # check if `t` is restored
        self.assertEqual(await client.query('.s.len();'), 3)

    async def test_return(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `return` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('return(1, 2 ,3);')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `return` expects argument 2 to be of '
                r'type `int` but got type `float` instead'):
            await client.query('return(nil, 0.0);')

        with self.assertRaisesRegex(
                BadDataError,
                r'expecting a `deep` value between 0 and 127 '
                r'but got 200 instead'):
            await client.query('return(nil, 200);')

        with self.assertRaisesRegex(
                BadDataError,
                r'expecting a `deep` value between 0 and 127 '
                r'but got -2 instead'):
            await client.query('return(nil, -2);')

        self.assertIs(await client.query(r'''
            return();
            "Not returned";
        '''), None)

        self.assertEqual(await client.query(r'''
            return(42);
            "Not returned";
        '''), 42)

        self.assertEqual(await client.query(r'''
            [0, 1, 2].map(|x| {
                return((x + 1));
                0;
            });
        '''), [1, 2, 3])

        self.assertEqual(await client.query(r'''
            (|x| {
                try(return((x + 1)));
                0;
            }).call(41);
        '''), 42)

        self.assertEqual(await client.query(r'''
            .a = 10;
            .a = return(11);  // Return, so do not overwrite a
        '''), 11)

        self.assertEqual(await client.query('.a'), 10)

    async def test_set_property(self, client):
        await client.query('.a = 1;')

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `set`'):
            await client.query('nil.set();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `set` takes 2 arguments but 0 were given'):
            await client.query('.set();')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `set` expects argument 1 to be of '
                r'type `raw` but got type `nil` instead'):
            await client.query('.set(nil, nil);')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change type `thing` while the value is being used'):
            await client.query('.map(||.set("a", 42));')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change or remove property `arr` on `#\d+` while '
                r'the `list` is being used'):
            await client.query(r'''
                .arr = ['a', 'b'];
                .arr.push({
                    .set('arr', [1, 2, 3]);
                    'c';
                })
            ''')

        self.assertEqual(await client.query('.set("a", 42);'), 42)
        self.assertEqual(await client.query('.a;'), 42)
        self.assertEqual(await client.query('.set("foo", "bar");'), "bar")
        self.assertEqual(await client.query('.foo;'), "bar")

    async def test_set_new_type(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `set`'):
            await client.query('nil.set();')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot convert type `nil` to `set`'):
            await client.query('set(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot add type `int` to a set'):
            await client.query(r'set([{}, thing(.id()), 3]);')

        self.assertEqual(await client.query('set();'), {'$': []})
        self.assertEqual(await client.query('set([]);'), {'$': []})
        self.assertEqual(await client.query('set(set());'), {'$': []})
        self.assertEqual(
            await client.query(r'set([{}, {}]);'),
            {'$': [{}, {}]})
        self.assertEqual(
            await client.query(r'set({}, {});'),
            {'$': [{}, {}]})
        self.assertEqual(
            await client.query(r'set({});'),
            {'$': [{}]})

    async def test_splice(self, client):
        await client.query('.li = [];')
        self.assertEqual(await client.query('.li.splice(0, 0, "a")'), [])
        self.assertEqual(await client.query('.li.splice(1, 0, "d", "e")'), [])
        self.assertEqual(await client.query('.li.splice(1, 0, "b", "d")'), [])
        self.assertEqual(await client.query('.li.splice(2, 1, "c")'), ['d'])
        self.assertEqual(await client.query('.li.splice(0, 2)'), ['a', 'b'])
        self.assertEqual(await client.query('.li;'), ['c', 'd', 'e'])

        self.assertEqual(await client.query('.li.splice(-2, 1)'), ['d'])
        self.assertEqual(await client.query('.li.splice(5, 1)'), [])
        self.assertEqual(await client.query('.li.splice(-5, 1)'), [])
        self.assertEqual(await client.query('.li.splice(0, -1)'), [])

        self.assertEqual(await client.query('.li;'), ['c', 'e'])
        self.assertEqual(await client.query('.li.splice(0, 10)'), ['c', 'e'])
        self.assertEqual(await client.query('.li.splice(5, 10, 1, 2, 3)'), [])
        self.assertEqual(await client.query('.li;'), [1, 2, 3])

        with self.assertRaisesRegex(
                IndexError,
                'type `thing` has no function `splice`'):
            await client.query('.splice();')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `splice`'):
            await client.query('a = [.li]; a[0].splice(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `splice` requires at least 2 arguments '
                'but 0 were given'):
            await client.query('.li.splice();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `splice` requires at least 2 arguments '
                'but 1 was given'):
            await client.query('.li.splice(0);')

        with self.assertRaisesRegex(
                BadDataError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.li.map(||.li.splice(0, 1));')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `splice` expects argument 1 to be of '
                r'type `int` but got type `float` instead'):
            await client.query('.li.splice(0.0, 0)')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `splice` expects argument 2 to be of '
                r'type `int` but got type `nil` instead'):
            await client.query('.li.splice(0, nil)')

    async def test_startswith(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `startswith`'):
            await client.query('nil.startswith("world!");')

        with self.assertRaisesRegex(
                BadDataError,
                'function `startswith` takes 1 argument '
                'but 2 were given'):
            await client.query('"Hi World!".startswith("x", 2)')

        with self.assertRaisesRegex(
                BadDataError,
                'function `startswith` takes 1 argument '
                'but 0 were given'):
            await client.query('"Hi World!".startswith()')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `startswith` expects argument 1 to be of '
                r'type `raw` but got type `list` instead'):
            await client.query('"Hi World!".startswith([]);')

        self.assertTrue(await client.query('"Hi World!".startswith("")'))
        self.assertFalse(await client.query('"".startswith("!")'))
        self.assertTrue(await client.query('"Hi World!".startswith("Hi")'))
        self.assertFalse(await client.query('"Hi World!".startswith("hi")'))

    async def test_str(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `str`'):
            await client.query('nil.str();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `str` takes at most 1 argument but 2 were given'):
            await client.query('str(1, 2);')

        self.assertEqual(await client.query('str();'), "")
        self.assertEqual(await client.query('str(3.14);'), "3.14")
        self.assertEqual(await client.query('str(3.14);'), "3.14")
        self.assertEqual(await client.query('str(-3.14);'), "-3.14")
        self.assertEqual(await client.query('str(42);'), "42")
        self.assertEqual(await client.query('str(-42);'), "-42")
        self.assertEqual(await client.query('str(true);'), "true")
        self.assertEqual(await client.query('str(false);'), "false")
        self.assertEqual(await client.query('str(nil);'), "nil")
        self.assertEqual(await client.query('str("abc");'), "abc")
        self.assertEqual(await client.query('str("");'), "")
        self.assertEqual(await client.query('str(/.*/i);'), "/.*/i")
        self.assertEqual(await client.query('str([]);'), "<array>")
        self.assertEqual(await client.query(r'str({});'), "<thing>")
        self.assertEqual(await client.query('str(||nil);'), "<closure>")

    async def test_syntax_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `syntax_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('syntax_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `syntax_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('syntax_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'syntax_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('syntax_err();')
        self.assertEqual(err['error_code'], EX_SYNTAX_ERROR)
        self.assertEqual(err['error_msg'], "syntax error in query")

        err = await client.query('syntax_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_SYNTAX_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_test(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `regex` has no function `test`'):
            await client.query('(/.*/).test();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `test` takes 1 argument but 0 were given'):
            await client.query('"".test();')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `test` expects argument 1 to be of '
                r'type `regex` but got type `raw` instead'):
            await client.query('"".test("abc");')

        self.assertTrue(await client.query(r'"".test(//);'))
        self.assertTrue(await client.query(r'"Hi".test(/hi/i);'))
        self.assertTrue(await client.query(r'"hello!".test(/hello.*/);'))
        self.assertFalse(await client.query(r'"Hi".test(/hi/);'))
        self.assertFalse(await client.query(r'"hello".test(/hello!.*/);'))

    async def test_thing(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `thing`'):
            await client.query('nil.thing();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `thing` takes at most 1 argument but 2 were given'):
            await client.query('thing(1, 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot convert type `regex` to `thing`'):
            await client.query('thing(/.*/);')

        with self.assertRaisesRegex(
                BadDataError,
                'cannot convert type `nil` to `thing`'):
            await client.query('thing(nil);')

        with self.assertRaisesRegex(
                IndexError,
                'collection `stuff` has no `thing` with id -1'):
            await client.query('thing(-1);')

        id = await client.query(r'(.t = {x:42}).id();')
        t = await client.query(f'thing({id});')
        self.assertEqual(t['x'], 42)
        self.assertEqual(await client.query('thing();'), {})
        stuff, t = await client.query(f'return([thing(.id()), #{id}], 2);')
        self.assertEqual(stuff['t'], t)
        self.assertTrue(await client.query(f'( #{id} == thing(#{id}) );'))

    async def test_try(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `try` requires at least 1 argument '
                'but 0 were given'):
            await client.query('try();')

        with self.assertRaisesRegex(
                BadDataError,
                r'function `try` expects arguments 2..X to be of '
                r'type `error` but got type `nil` instead'):
            await client.query('try((10 //0), nil);')

        self.assertIs(await client.query('iserr(try( (10 // 2) ));'), False)
        self.assertIs(await client.query('iserr(try( (10 // 0) ));'), True)
        self.assertIs(await client.query(
            'iserr(try( (10 // 0), zero_div_err() ));'), True)

        with self.assertRaisesRegex(
                ZeroDivisionError,
                'division or modulo by zero'):
            await client.query('try( (10 // 0), index_err());')

    async def test_type(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `raw` has no function `type`'):
            await client.query('"".type();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `type` takes 1 argument but 0 were given'):
            await client.query('type();')

        self.assertEqual(await client.query('type(nil);'), "nil")
        self.assertEqual(await client.query('type(true);'), "bool")
        self.assertEqual(await client.query('type(1);'), "int")
        self.assertEqual(await client.query('type(0.0);'), "float")
        self.assertEqual(await client.query('type("Hi");'), "raw")
        self.assertEqual(await client.query('type([]);'), "list")
        self.assertEqual(await client.query('type([[]][0]);'), "tuple")
        self.assertEqual(await client.query(r'type({});'), "thing")
        self.assertEqual(await client.query('type(set());'), "set")
        self.assertEqual(await client.query('type(||nil);'), "closure")
        self.assertEqual(await client.query('type(//);'), "regex")

    async def test_upper(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `upper`'):
            await client.query('nil.upper();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `upper` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".upper(nil);')

        self.assertEqual(await client.query('"".upper();'), "")
        self.assertEqual(await client.query('"U".upper();'), "U")
        self.assertEqual(await client.query('"hi !!".upper();'), "HI !!")

    async def test_values(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `values`'):
            await client.query('nil.values();')

        with self.assertRaisesRegex(
                IndexError,
                'function `values` is undefined'):
            await client.query('values();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `values` takes 0 arguments but 1 was given'):
            await client.query('.values(nil);')

        self.assertEqual(await client.query(r'{}.values();'), [])
        self.assertEqual(await client.query(r'{a:1}.values();'), [1])
        self.assertEqual(await client.query(r'{a:1, b:2}.values();'), [1, 2])

    async def test_wse(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `wse`'):
            await client.query('nil.wse();')

        with self.assertRaisesRegex(
                BadDataError,
                'function `wse` takes 1 argument but 0 were given'):
            await client.query('wse();')

        res = await client.query(r'''
            wse({
                x = 0;
                a = |v| x += v;
                [1 ,2 ,3, 4].map(a);
                x;
            });
        ''')
        self.assertEqual(res, 10)

    async def test_zero_div_err(self, client):
        with self.assertRaisesRegex(
                BadDataError,
                'function `zero_div_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('zero_div_err("bla", 2);')

        with self.assertRaisesRegex(
                BadDataError,
                'function `zero_div_err` expects argument 1 to be of '
                'type `raw` but got type `nil` instead'):
            await client.query('zero_div_err(nil);')

        with self.assertRaisesRegex(
                BadDataError,
                'error messages must have valid UTF8 encoding'):
            await client.query(
                'zero_div_err(blob(0));',
                blobs=(pickle.dumps({}), ))

        err = await client.query('zero_div_err();')
        self.assertEqual(err['error_code'], EX_ZERO_DIV)
        self.assertEqual(err['error_msg'], "division or module by zero")

        err = await client.query('zero_div_err("my custom error msg");')
        self.assertEqual(err['error_code'], EX_ZERO_DIV)
        self.assertEqual(err['error_msg'], "my custom error msg")


if __name__ == '__main__':
    run_test(TestCollectionFunctions())
