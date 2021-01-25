#!/usr/bin/env python
import asyncio
import pickle
import time
import base64
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
from thingsdb.exceptions import ThingsDBError
from thingsdb.client.protocol import Err


class TestCollectionFunctions(TestBase):

    title = 'Test collection scope functions'

    @default_test_setup(num_nodes=2, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)
        # return  # uncomment to skip garbage collection test

        # add another node so away node and gc is forced
        await self.node1.join_until_ready(client)

        # expected no garbage collection
        counters = await client.query('counters();', scope='@node')
        self.assertEqual(counters['garbage_collected'], 0)

    async def test_unknown(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'function `unknown` is undefined'):
            await client.query('unknown();')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `unknown`'):
            await client.query('nil.unknown();')

        with self.assertRaisesRegex(
                LookupError,
                'function `new_node` is undefined in the `@collection` scope; '
                'you might want to query the `@thingsdb` scope?'):
            await client.query('new_node();')

        with self.assertRaisesRegex(
                LookupError,
                'function `counters` is undefined in the `@collection` scope; '
                'you might want to query a `@node` scope?'):
            await client.query('counters();')

        self.assertIs(await client.query(r'''

        '''), None)

    async def test_alt_raise(self, client):

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `alt_raise`'):
            await client.query('nil.alt_raise(nil, -100);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `alt_raise` takes at most 3 arguments '
                'but 4 were given'):
            await client.query('alt_raise(nil, -100, "msg", nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `alt_raise` requires at least 2 arguments '
                'but 0 were given'):
            await client.query('alt_raise();')

        with self.assertRaisesRegex(
                TypeError,
                'function `alt_raise` expects argument 2 to be of type `int` '
                'but got type `nil` instead'):
            await client.query('alt_raise(1/0, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `alt_raise` expects argument 3 to be of type `str` '
                'but got type `nil` instead'):
            await client.query('alt_raise(1/0, -100, nil);')

        with self.assertRaisesRegex(
                ValueError,
                'function `alt_raise` expects an error code '
                'between -127 and -50 but got 100 instead'):
            await client.query('alt_raise(42/0, 100);')

        with self.assertRaisesRegex(
                ValueError,
                'function `alt_raise` expects an error code '
                'between -127 and -50 but got -128 instead'):
            await client.query('alt_raise(42/0, -128);')

        with self.assertRaisesRegex(
                ThingsDBError,
                'division or modulo by zero'):
            await client.query('alt_raise(42/0, -100);')

        with self.assertRaisesRegex(
                ThingsDBError,
                'just a message'):
            await client.query('alt_raise(42/0, -100, "just a message");')

        try:
            await client.query('alt_raise(42/0, -50);')
        except ThingsDBError as e:
            self.assertEqual(e.error_code, -50)
            self.assertEqual(str(e), 'division or modulo by zero')

        try:
            await client.query('alt_raise(42/0, -127, "just a message");')
        except ThingsDBError as e:
            self.assertEqual(e.error_code, -127)
            self.assertEqual(str(e), 'just a message')

        self.assertEqual(
            await client.query('alt_raise(42/6, -100, "just a message");'),
            7
        )

    async def test_add(self, client):
        await client.query(r'.s = set(); .a = {}; .b = {}; .c = {};')
        self.assertEqual(
            await client.query('[.s.add(.a, .b), .s.len()]'), [2, 2])
        self.assertEqual(
            await client.query('[.s.add(.b, .c), .s.len()]'), [1, 3])
        self.assertEqual(
            await client.query(r'[.s.add({}), .s.len()]'), [1, 4])

        with self.assertRaisesRegex(
                TypeError,
                'cannot add type `nil` to a set'):
            await client.query(r'.s.add(.a, .b, {}, nil);')

    async def test_list(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `list`'):
            await client.query('nil.list();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `list` takes at most 1 argument but 2 were given'):
            await client.query('list(1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `nil` to `list`'):
            await client.query('list(nil);')

        self.assertEqual(await client.query('list();'), [])
        self.assertEqual(await client.query('list( [] );'), [])
        self.assertEqual(await client.query(r'list(set([{}]));'), [{}])

    async def test_assert(self, client):
        with self.assertRaisesRegex(
                AssertionError,
                r'assertion statement `\(1>2\)` has failed'):
            await client.query('assert((1>2));')

        try:
            await client.query('assert((1>2), "my custom message");')
        except AssertionError as e:
            self.assertEqual(str(e), 'my custom message')
            self.assertEqual(e.error_code, Err.EX_ASSERT_ERROR)
        else:
            raise Exception('AssertionError not raised')

        self.assertIs(await client.query('''
            assert(2>1);
        '''), None)

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `assert` requires at least 1 argument '
                'but 0 were given'):
            await client.query('assert();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `assert` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('assert(true, "x", 1);')

        with self.assertRaisesRegex(
                TypeError,
                'function `assert` expects argument 2 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('assert(false, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `assert` expects argument 2 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'assert(false, blob);',
                blob=pickle.dumps({}))

    async def test_type_assert(self, client):
        with self.assertRaisesRegex(
                TypeError,
                r'invalid type `str`'):
            await client.query('type_assert("x", "int");')

        with self.assertRaisesRegex(
                TypeError,
                r'invalid type `str`'):
            await client.query('type_assert("x", ["int", "float"]);')

        try:
            await client.query('type_assert(1, "str", "my custom message");')
        except TypeError as e:
            self.assertEqual(str(e), 'my custom message')
            self.assertEqual(e.error_code, Err.EX_TYPE_ERROR)
        else:
            raise Exception('AssertionError not raised')

        self.assertEqual(await client.query('type_assert(2, "int"); 4;'), 4)
        self.assertEqual(await client.query('''
            type_assert(2, ["str", "int"]);
            "ok";
        '''), "ok")

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `type_assert` requires at least 2 arguments '
                'but 0 were given'):
            await client.query('type_assert();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `type_assert` takes at most 3 arguments '
                'but 4 were given'):
            await client.query('type_assert(true, "bool", 1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `type_assert` expects argument 2 to be of '
                r'type `str`, type `list` or type `tuple` but got '
                r'type `nil` instead;'):
            await client.query('type_assert(false, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `type_assert` expects argument 2 to be a list '
                r'or tuple of type `str` but got type `int` instead;'):
            await client.query('''
                type_assert(1, [4, "int"]);
            ''')

    async def test_assert_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `assert_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('assert_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `assert_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('assert_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `assert_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'assert_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('assert_err();')
        self.assertEqual(err['error_code'], Err.EX_ASSERT_ERROR)
        self.assertEqual(err['error_msg'], "assertion statement has failed")

        err = await client.query('assert_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_ASSERT_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_auth_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `auth_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('auth_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `auth_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('auth_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `auth_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'auth_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('auth_err();')
        self.assertEqual(err['error_code'], Err.EX_AUTH_ERROR)
        self.assertEqual(err['error_msg'], "authentication error")

        err = await client.query('auth_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_AUTH_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_bad_data_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `bad_data_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('bad_data_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `bad_data_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('bad_data_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `bad_data_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'bad_data_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('bad_data_err();')
        self.assertEqual(err['error_code'], Err.EX_BAD_DATA)
        self.assertEqual(
            err['error_msg'],
            "unable to handle request due to invalid data")

        err = await client.query('bad_data_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_BAD_DATA)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_base64_decode(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `base64_decode`'):
            await client.query('"".base64_decode();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `base64_decode` takes 1 argument '
                'but 0 were given'):
            await client.query('base64_decode();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `base64_decode` expects argument 1 to be of '
                r'type `str` or type `bytes` but got type `int` instead'):
            await client.query('base64_decode(1);')

        self.assertEqual(await client.query('base64_decode("");'), b'')
        self.assertEqual(
            await client.query('base64_decode("VGhpbmdzREI=");'),
            b'ThingsDB')
        self.assertEqual(
            await client.query('base64_decode("VGhpbmdzREI");'),
            b'ThingsDB')

    async def test_base64_encode(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `base64_encode`'):
            await client.query('"".base64_encode();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `base64_encode` takes 1 argument '
                'but 0 were given'):
            await client.query('base64_encode();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `base64_encode` expects argument 1 to be of '
                r'type `str` or type `bytes` but got type `int` instead'):
            await client.query('base64_encode(1);')

        self.assertEqual(await client.query('base64_encode("");'), '')
        self.assertEqual(
            await client.query('base64_encode("ThingsDB");'),
            'VGhpbmdzREI=')

        src = b'TEST: ThingsDB\r\r\nDATE: 2019/12/02\r\r\n'
        enc = base64.b64encode(src)
        self.assertEqual(
            await client.query(f'base64_decode(e);', e=enc), src)
        self.assertEqual(
            await client.query(f'bytes(base64_encode(s));', s=src), enc)

    async def test_bool(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `bool`'):
            await client.query('nil.bool();')

        with self.assertRaisesRegex(
                NumArgumentsError,
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
        self.assertTrue(await client.query(r'bool(/.*/);'))
        self.assertTrue(await client.query('bool(||nil);'))

    async def test_def(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `def`'):
            await client.query('nil.def();')

        with self.assertRaisesRegex(
                LookupError,
                'function `def` is undefined'):
            await client.query('def();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `def` takes 0 arguments but 1 was given'):
            await client.query('(||nil).def(1);')

        self.assertEqual(await client.query('(||{nil}).def();'), r'''
|| {
    nil;
}
'''.strip())

        self.assertEqual(await client.query(r'''
            (|| range(10)[0:9:2]).def();
        '''), "|| range(10)[0:9:2]")

    async def test_code(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `code`'):
            await client.query('nil.code();')

        with self.assertRaisesRegex(
                LookupError,
                'function `code` is undefined'):
            await client.query('code();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `code` takes 0 arguments but 2 were given'):
            await client.query('type_err().code(1, 2);')

        self.assertEqual(await client.query('type_err().code();'), -61)

    async def test_msg(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `msg`'):
            await client.query('nil.msg();')

        with self.assertRaisesRegex(
                LookupError,
                'function `msg` is undefined'):
            await client.query('msg();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `msg` takes 0 arguments but 2 were given'):
            await client.query('type_err().msg(1, 2);')

        self.assertEqual(
            await client.query('type_err().msg();'),
            'object of inappropriate type')

    async def test_call(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `call`'):
            await client.query('nil.call();')

    async def test_call(self, client):
        with self.assertRaisesRegex(
                TypeError,
                'variable `unknown` is of type `nil` and is not callable'):
            await client.query('unknown = nil; unknown();')

        with self.assertRaisesRegex(
                LookupError,
                'function `call` is undefined'):
            await client.query('call();')

        with self.assertRaisesRegex(
                OperationError,
                r'stored closures with side effects must be wrapped '
                r'using `wse\(...\)`'):
            await client.query(r'''
                .test = |x| .x = x;
                .test.call(42);
            ''')

        with self.assertRaisesRegex(
                OperationError,
                r'stored closures with side effects must be wrapped '
                r'using `wse\(...\)`'):
            await client.query(r'''
                .test = |x| .x = x;
                .test(42);
            ''')

        self.assertIs(await client.query('(||nil).call(nil);'), None)
        self.assertIs(await client.query('fun = ||nil; fun(nil);'), None)
        self.assertEqual(await client.query('(|y|.y = y).call(42);'), 42)
        self.assertEqual(await client.query('wse(.test.call(42));'), 42)

        self.assertEqual(await client.query('.x'), 42)
        self.assertEqual(await client.query('.y'), 42)

        self.assertEqual(await client.query('f = (|y|.y = y); f(666);'), 666)
        self.assertEqual(await client.query('wse(.test(666));'), 666)

        self.assertEqual(await client.query('.x'), 666)
        self.assertEqual(await client.query('.y'), 666)

    async def test_contains(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `contains`'):
            await client.query('nil.contains("world!");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `contains` takes 1 argument but 2 were given'):
            await client.query('"Hi World!".contains("x", 2)')

        with self.assertRaisesRegex(
                TypeError,
                r'function `contains` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
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
                LookupError,
                'type `nil` has no function `deep`'):
            await client.query('nil.deep();')

        with self.assertRaisesRegex(
                NumArgumentsError,
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

    async def test_event_id(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `event_id`'):
            await client.query('nil.event_id();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `event_id` takes 0 arguments but 1 was given'):
            await client.query('event_id(nil);')

        self.assertIs(await client.query('event_id();'), None)
        self.assertIsInstance(await client.query('wse(event_id());'), int)

    async def test_del(self, client):
        await client.query(r'.greet = "Hello world";')

        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `del`'):
            await client.query('.greet.del("x");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `del` takes 1 argument '
                'but 0 were given'):
            await client.query('.del();')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `thing` while the value is being used'):
            await client.query('.map( ||.del("greet") );')

        with self.assertRaisesRegex(
                TypeError,
                r'function `del` expects argument 1 to be of type `str` '
                r'but got type `nil` instead'):
            await client.query('.del(nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'property name must follow the naming rules'):
            await client.query('.del("");')

        with self.assertRaisesRegex(
                LookupError,
                r'thing `#\d+` has no property `x`'):
            await client.query('.del("x");')

        self.assertEqual(await client.query(r'.del("greet");'), "Hello world")
        self.assertFalse(await client.query(r'.has("greet");'))

    async def test_assign(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `list` has no function `assign`'):
            await client.query(r'[].assign({});')

        with self.assertRaisesRegex(
                LookupError,
                'function `assign` is undefined'):
            await client.query(r'assign({});')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `assign` takes 1 argument '
                'but 0 were given'):
            await client.query('.assign();')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `thing` while the value is being used'):
            await client.query('.x = 1; .map(|| .assign( {} ) );')

        with self.assertRaisesRegex(
                TypeError,
                r'function `assign` expects argument 1 to be of type `thing` '
                r'but got type `nil` instead'):
            await client.query('.assign(nil);')

        self.assertEqual(await client.query(r'{a: 1}.assign({});'), {"a": 1})
        self.assertEqual(await client.query(r'.assign({a: 42}); .a;'), 42)
        res = await client.query(r'''
            set_type('Person', {name: 'str'});
            .p1 = Person{};
            .p2 = Person{};
            .t1 = {a: 1, b: 2};
            .t2 = {a: 1, b: 2};
            .p1.assign({name: 'Iris'});
            .p2.assign(.p1);
            .t1.assign({
                b: 4, c: 5
            });
            .t2.assign(.p1);
            [.p1.name, .p2.name, .t1.filter(||true), .t2.filter(||true)];
        ''')
        self.assertEqual(res, [
            'Iris',
            'Iris',
            {"a": 1, "b": 4, "c": 5},
            {"a": 1, "b": 2, "name": 'Iris'}
        ])

        res = await client.query(r'''
            .p = Person{name: 'Iris'};
            try(
                .p.assign({
                    name: 4,
                    age: 5,
                })
            );
            .p.filter(||true);
        ''')
        self.assertEqual(res, {"name": "Iris"})

    async def test_emit(self, client):
        await client.query(r'.greet = "Hello world";')

        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `emit`'):
            await client.query('.greet.emit("x");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `emit` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.emit();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `emit` expects the `event` argument to be of '
                r'type `str` but got type `nil` instead'):
            await client.query('.emit(nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a `deep` value between 0 and 127 '
                r'but got 200 instead'):
            await client.query('.emit(200, "a");')

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a `deep` value between 0 and 127 '
                r'but got -2 instead'):
            await client.query('.emit(-2, "a");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `emit` expects the `event` argument to be of '
                r'type `str` but got type `nil` instead'):
            await client.query('.emit(nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `emit` expects the `event` argument to be of '
                r'type `str` but got type `int` instead'):
            await client.query('.emit(0, 1);')

        self.assertIs(await client.query(r'.emit("greet");'), None)
        await client.query(r'.bob = {};')
        await client.query(r'.bob.emit("msg", .del("bob"));')

    async def test_doc(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `doc`'):
            await client.query('"abc".doc();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `doc` takes 0 arguments '
                'but 2 were given'):
            await client.query('(||nil).doc(1, 2);')

        self.assertEqual(await client.query('(||{"test!"}).doc();'), 'test!')
        self.assertEqual(await client.query('(||nil).doc();'), '')
        self.assertEqual(await client.query(r'''
            (||wse(return({
                "Test!";
                nil;
            }, 2))).doc();
        '''), 'Test!')

    async def test_ends_with(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `ends_with`'):
            await client.query('nil.ends_with("world!");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `ends_with` takes 1 argument but 2 were given'):
            await client.query('"Hi World!".ends_with("x", 2)')

        with self.assertRaisesRegex(
                TypeError,
                r'function `ends_with` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('"Hi World!".ends_with(1);')

        self.assertTrue(await client.query('"Hi World!".ends_with("")'))
        self.assertFalse(await client.query('"".ends_with("!")'))
        self.assertTrue(await client.query('"Hi World!".ends_with("World!")'))
        self.assertFalse(await client.query('"Hi World!".ends_with("world!")'))

    async def test_extend(self, client):
        await client.query('.list = [];')
        self.assertEqual(await client.query('.list.extend(["a"])'), 1)
        self.assertEqual(await client.query('.list.extend(["b", "c"])'), 3)
        self.assertEqual(await client.query('.list;'), ['a', 'b', 'c'])

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `extend`'):
            await client.query('nil.extend();')

        with self.assertRaisesRegex(
                LookupError,
                'function `extend` is undefined'):
            await client.query('extend();')

        with self.assertRaisesRegex(
                LookupError,
                'type `tuple` has no function `extend`'):
            await client.query('.a = [.list]; .a[0].extend(nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `extend` takes 1 argument '
                'but 0 were given'):
            await client.query('.list.extend();')

        with self.assertRaisesRegex(
                TypeError,
                'function `extend` expects argument 1 to be of '
                'type `list` or type `tuple` but got type `nil` instead'):
            await client.query('.list.extend(nil);')

        with self.assertRaisesRegex(
                OperationError,
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
                LookupError,
                'type `nil` has no function `filter`'):
            await client.query('nil.filter(||true);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `filter` takes 1 argument but 0 were given'):
            await client.query('.filter();')

        with self.assertRaisesRegex(
                TypeError,
                'function `filter` expects argument 1 to be of type `closure` '
                'but got type `nil` instead'):
            await client.query('.filter(nil);')

        self.assertEqual(
            (await client.query('.iris.filter(|k|(k == "name"));')),
            {'name': 'Iris'})

        self.assertEqual(
            (await client.query('.iris.filter(|_k, v|(v == 6));')),
            {'age': 6})

        self.assertEqual(
            (await client.query('.iris.likes.filter(|v|is_str(v));')),
            ['k3', 'swimming', 'red'])

        self.assertEqual(
            (await client.query('.iris.likes.filter(|_v, i|(i > 1));')),
            ['red', 6])

        self.assertEqual(
            (await client.query('.girls.filter(|v|(v.age == 6))'))
            [0]['age'],
            6)

        self.assertEqual(
            (await client.query(
                '.girls.filter(|_v, i|(i == .cato.id()))'))[0]['age'],
            5)

        self.assertEqual(await client.query('.iris.filter(||nil);'), {})
        self.assertEqual(await client.query('.iris.likes.filter(||nil);'), [])
        self.assertEqual(await client.query(r'{}.filter(||true)'), {})
        self.assertEqual(await client.query(r'[].filter(||true)'), [])
        self.assertEqual(await client.query(r'set().filter(||1)'), [])

    async def test_reverse(self, client):
        await client.query(r'''
            .arr1 = range(10);
            arr2 = [{n: 2}, {n: 1}, {n: 0}];
            set_type('List', {numbers: '[int]'});
            .list = List{
                numbers: [1, 2, 3]
            };
            .arr2 = arr2.reverse();
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `reverse`'):
            await client.query('nil.reverse();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `reverse` takes 0 arguments '
                'but 1 was given'):
            await client.query('.arr1.reverse(nil);')

        self.assertEqual(
            await client.query('.arr1.reverse();'),
            list(range(10))[::-1]
        )

        self.assertEqual(
            await client.query(r'''
                arr = .list.numbers.reverse();
                arr.push("abc");
                arr;
            '''),
            [3, 2, 1, "abc"]
        )

        res = await client.query('.list.numbers = .list.numbers.reverse()')
        self.assertEqual(res, [3, 2, 1])

        with self.assertRaisesRegex(
                TypeError,
                'type `str` is not allowed in restricted array'):
            await client.query('.list.numbers.push("abc");')

        res = await client.query('.arr2;')
        self.assertEqual(len(res), 3)

        for i, item in enumerate(res):
            self.assertIn('#', item.keys())
            self.assertEqual(item['n'], i)

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
                LookupError,
                'type `nil` has no function `find`'):
            await client.query('nil.find(||true);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `find` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.x.find();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `find` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('.x.find(||true, 2, 3);')

        with self.assertRaisesRegex(
                TypeError,
                'function `find` expects argument 1 to be of type `closure` '
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

    async def test_find_index(self, client):
        await client.query(r'.x = [42, ""];')

        with self.assertRaisesRegex(
                LookupError,
                'type `int` has no function `find_index`'):
            await client.query('(1).find_index(||true);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `find_index` takes 1 argument but 2 were given'):
            await client.query('.x.find_index(0, 1);')

        with self.assertRaisesRegex(
                TypeError,
                'function `find_index` expects argument 1 to be of '
                'type `closure` but got type `bool` instead'):
            await client.query('.x.find_index(true);')

        self.assertIs(await client.query('[].find_index(||true);'), None)
        self.assertIs(await client.query('[].find_index(||false);'), None)
        self.assertEqual(await client.query('.x.find_index(||true);'), 0)
        self.assertIs(await client.query('.x.find_index(||false);'), None)
        self.assertEqual(await client.query('.x.find_index(|_,i|(i>0));'), 1)
        self.assertEqual(await client.query('.x.find_index(|v|(v==42));'), 0)
        self.assertEqual(await client.query('.x.find_index(|v|is_str(v));'), 1)

    async def test_float(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `float`'):
            await client.query('nil.float();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `float` takes at most 1 argument but 2 were given'):
            await client.query('float(1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `closure` to `float`'):
            await client.query('float(||nil);')

        with self.assertRaisesRegex(
                TypeError,
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
                NumArgumentsError,
                'function `forbidden_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('forbidden_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `forbidden_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('forbidden_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `forbidden_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'forbidden_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('forbidden_err();')
        self.assertEqual(err['error_code'], Err.EX_FORBIDDEN)
        self.assertEqual(err['error_msg'], "forbidden (access denied)")

        err = await client.query('forbidden_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_FORBIDDEN)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_get(self, client):
        await client.query(r'.iris = {name: "Iris"};')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `get`'):
            await client.query('nil.get();')

        with self.assertRaisesRegex(
                LookupError,
                'function `get` is undefined'):
            await client.query('get();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `get` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.get();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `get` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('.get("x", nil, nil);')

        self.assertIs(await client.query('.iris.get("x");'), None)
        self.assertEqual(await client.query('.iris.get("x", "?");'), '?')
        self.assertEqual(await client.query('.iris.get("name");'), 'Iris')

    async def test_has_type(self, client):
        await client.query(r'''new_type('Ti')''')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has_type`'):
            await client.query('nil.has_type();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has_type` takes 1 argument but 0 were given'):
            await client.query('has_type();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has_type` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('has_type(0);')

        self.assertTrue(await client.query(r'''has_type('Ti');'''))
        self.assertFalse(await client.query(r'''has_type('ti');'''))

    async def test_has_set(self, client):
        await client.query(r'''
            .iris = {name: "Iris"};
            .cato = {name: "Cato"};
            .s = set([.iris]);
        ''')

        with self.assertRaisesRegex(
                LookupError,
                'function `has` is undefined'):
            await client.query('has(.iris);')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `has`'):
            await client.query('nil.has(.iris);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has` takes 1 argument but 0 were given'):
            await client.query('.s.has();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has` expects argument 1 to be of '
                r'type `thing` but got type `int` instead'):
            await client.query('.s.has(0);')

        self.assertTrue(await client.query('.s.has(.iris);'))
        self.assertFalse(await client.query('.s.has(.cato);'))

    async def test_has_thing(self, client):
        await client.query(r'.x = 0.0;')

        with self.assertRaisesRegex(
                LookupError,
                'function `has` is undefined'):
            await client.query('has("x");')

        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `has`'):
            await client.query('.x.has("x");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has` takes 1 argument but 0 were given'):
            await client.query('.has();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `has` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
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
                LookupError,
                'type `nil` has no function `id`'):
            await client.query('nil.id();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `id` takes 0 arguments but 1 was given'):
            await client.query('.id(nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `id` takes 0 arguments but 2 were given'):
            await client.query('.id(nil, nil);')

    async def test_index_of(self, client):
        await client.query(
            r'.x = [42, "thingsdb", thing(.id()), 42, false, nil];')

        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `index_of`'):
            await client.query('(1.0).index_of("x");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `index_of` takes 1 argument but 0 were given'):
            await client.query('.x.index_of();')

        self.assertEqual(await client.query('.x.index_of(42);'), 0)
        self.assertEqual(await client.query('.x.index_of("thingsdb");'), 1)
        self.assertEqual(await client.query('.x.index_of(thing(.id()));'), 2)
        self.assertEqual(await client.query('.x.index_of(false);'), 4)
        self.assertEqual(await client.query('.x.index_of(nil);'), 5)
        self.assertIs(await client.query('.x.index_of(42.1);'), None)
        self.assertIs(await client.query('.x.index_of("ThingsDb");'), None)
        self.assertIs(await client.query('.x.index_of(true);'), None)
        self.assertIs(await client.query(r'.x.index_of({});'), None)

        # cleanup garbage, the reference to the collection
        await client.query(r'''.x.splice(2, 1);''')

    async def test_has_list(self, client):
        await client.query(
            r'.x = [42, "thingsdb", thing(.id()), 42, false, nil];')

        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `has`'):
            await client.query('(1.0).has("x");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has` takes 1 argument but 0 were given'):
            await client.query('.x.has();')

        self.assertEqual(await client.query('.x.has(42);'), True)
        self.assertEqual(await client.query('.x.has("thingsdb");'), True)
        self.assertEqual(await client.query('.x.has(thing(.id()));'), True)
        self.assertEqual(await client.query('.x.has(false);'), True)
        self.assertEqual(await client.query('.x.has(nil);'), True)
        self.assertIs(await client.query('.x.has(42.1);'), False)
        self.assertIs(await client.query('.x.has("ThingsDb");'), False)
        self.assertIs(await client.query('.x.has(true);'), False)
        self.assertIs(await client.query(r'.x.has({});'), False)

        # cleanup garbage, the reference to the collection
        await client.query(r'''.x.splice(2, 1);''')

    async def test_lookup_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `lookup_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('lookup_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `lookup_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('lookup_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `lookup_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'lookup_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('lookup_err();')
        self.assertEqual(err['error_code'], Err.EX_LOOKUP_ERROR)
        self.assertEqual(err['error_msg'], "requested resource not found")

        err = await client.query('lookup_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_LOOKUP_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_if(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `if`'):
            await client.query('nil.if();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `if` requires at least 2 arguments '
                'but 0 were given'):
            await client.query('if();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `if` takes at most 3 arguments but 4 were given'):
            await client.query('if(1, 2, 3, 4);')

        self.assertIs(await client.query('if(2 > 1, a=1, a=0);'), None)
        self.assertEqual(await client.query('if(2>1, a=1, a=0); a;'), 1)
        self.assertEqual(await client.query('if(1>2, a=1, a=0); a;'), 0)
        self.assertEqual(await client.query('a=2; if(2>1, a=1); a;'), 1)
        self.assertEqual(await client.query('a=0; if(1>2, a=1); a;'), 0)

    async def test_int(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `int`'):
            await client.query('nil.int();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `int` takes at most 1 argument but 2 were given'):
            await client.query('int(1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `regex` to `int`'):
            await client.query('int(/.*/);')

        with self.assertRaisesRegex(
                TypeError,
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

    async def test_is_array(self, client):
        await client.query('.x = [[0, 1], nil]; .y = .x[0];')
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_array` takes 1 argument but 0 were given'):
            await client.query('is_array();')

        self.assertTrue(await client.query('is_array([]);'))
        self.assertTrue(await client.query('is_array(.x);'))
        self.assertTrue(await client.query('is_array(.y);'))
        self.assertTrue(await client.query('is_array(.x[0]);'))
        self.assertFalse(await client.query('is_array(0);'))
        self.assertFalse(await client.query('is_array("test");'))
        self.assertFalse(await client.query(r'is_array({});'))
        self.assertFalse(await client.query('is_array(.x[1]);'))
        self.assertFalse(await client.query('is_array(set());'))

    async def test_is_ascii(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_ascii` takes 1 argument but 0 were given'):
            await client.query('is_ascii();')

        self.assertTrue(await client.query('is_ascii( "pi" ); '))
        self.assertTrue(await client.query('is_ascii( "" ); '))
        self.assertFalse(await client.query('is_ascii( "ԉ" ); '))
        self.assertFalse(await client.query('is_ascii([]);'))
        self.assertFalse(await client.query('is_ascii(nil);'))
        self.assertFalse(await client.query(
                'is_ascii(blob);',
                blob=pickle.dumps('binary')))

    async def test_is_bool(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_bool` takes 1 argument but 3 were given'):
            await client.query('is_bool(1, 2, 3);')

        self.assertTrue(await client.query('is_bool( true ); '))
        self.assertTrue(await client.query('is_bool( false ); '))
        self.assertTrue(await client.query('is_bool( is_int(.id()) ); '))
        self.assertFalse(await client.query('is_bool( "ԉ" ); '))
        self.assertFalse(await client.query('is_bool([]);'))
        self.assertFalse(await client.query('is_bool(nil);'))

    async def test_is_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_err` takes 1 argument but 2 were given'):
            await client.query('is_err(1, 2);')

        self.assertTrue(await client.query('is_err( err() ); '))
        self.assertTrue(await client.query('is_err( zero_div_err() ); '))
        self.assertTrue(await client.query('is_err( try ((1/0)) ); '))
        self.assertFalse(await client.query('is_err( "ԉ" ); '))
        self.assertFalse(await client.query('is_err([]);'))
        self.assertFalse(await client.query('is_err(nil);'))

    async def test_is_float(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_float` takes 1 argument but 0 were given'):
            await client.query('is_float();')

        self.assertTrue(await client.query('is_float( 0.0 ); '))
        self.assertTrue(await client.query('is_float( -0.0 ); '))
        self.assertTrue(await client.query('is_float( inf ); '))
        self.assertTrue(await client.query('is_float( -inf ); '))
        self.assertTrue(await client.query('is_float( nan ); '))
        self.assertFalse(await client.query('is_float( "ԉ" ); '))
        self.assertFalse(await client.query('is_float( 42 );'))
        self.assertFalse(await client.query('is_float( nil );'))

    async def test_is_future(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_future` takes 1 argument but 0 were given'):
            await client.query('is_future();')

        self.assertTrue(await client.query('is_future(future(nil));'))
        self.assertTrue(await client.query(
            'is_future(future(nil).then(||nil));'))
        self.assertTrue(await client.query(
            'is_future(future(nil).else(||nil));'))
        self.assertFalse(await client.query('is_future([future(nil)][0]); '))
        self.assertFalse(await client.query('is_future( 42 );'))
        self.assertFalse(await client.query('is_future( nil );'))

    async def test_is_inf(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_inf` takes 1 argument but 0 were given'):
            await client.query('is_inf();')

        with self.assertRaisesRegex(
                TypeError,
                'function `is_inf` expects argument 1 to be of '
                'type `float` but got type `nil` instead'):
            await client.query('is_inf(nil);')

        self.assertTrue(await client.query('is_inf( inf ); '))
        self.assertTrue(await client.query('is_inf( -inf ); '))
        self.assertFalse(await client.query('is_inf( 0.0 ); '))
        self.assertFalse(await client.query('is_inf( nan ); '))
        self.assertFalse(await client.query('is_inf( 42 ); '))
        self.assertFalse(await client.query('is_inf( true ); '))

    async def test_is_int(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_int` takes 1 argument but 0 were given'):
            await client.query('is_int();')

        self.assertTrue(await client.query('is_int( 0 ); '))
        self.assertTrue(await client.query('is_int( -0 ); '))
        self.assertTrue(await client.query('is_int( 42 );'))
        self.assertFalse(await client.query('is_int( 0.0 ); '))
        self.assertFalse(await client.query('is_int( -0.0 ); '))
        self.assertFalse(await client.query('is_int( inf ); '))
        self.assertFalse(await client.query('is_int( -inf ); '))
        self.assertFalse(await client.query('is_int( nan ); '))
        self.assertFalse(await client.query('is_int( "ԉ" ); '))
        self.assertFalse(await client.query('is_int( nil );'))
        self.assertFalse(await client.query('is_int( set() );'))

    async def test_is_list(self, client):
        await client.query('.x = [[0, 1], nil]; .y = .x[0];')
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_list` takes 1 argument but 0 were given'):
            await client.query('is_list();')

        self.assertTrue(await client.query('is_list([]);'))
        self.assertTrue(await client.query('is_list(.x);'))
        self.assertTrue(await client.query('is_list(.y);'))
        self.assertFalse(await client.query('is_list(.x[0]);'))
        self.assertFalse(await client.query('is_list(0);'))
        self.assertFalse(await client.query('is_list("test");'))
        self.assertFalse(await client.query(r'is_list({});'))
        self.assertFalse(await client.query('is_list(.x[1]);'))
        self.assertFalse(await client.query('is_list(set());'))

    async def test_is_nan(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_nan` takes 1 argument but 0 were given'):
            await client.query('is_nan();')

        self.assertFalse(await client.query('is_nan( inf ); '))
        self.assertFalse(await client.query('is_nan( -inf ); '))
        self.assertFalse(await client.query('is_nan( 3.14 ); '))
        self.assertFalse(await client.query('is_nan( 42 ); '))
        self.assertFalse(await client.query('is_nan( true ); '))
        self.assertTrue(await client.query('is_nan( nan ); '))
        self.assertTrue(await client.query('is_nan( [] ); '))
        self.assertTrue(await client.query(r'is_nan( {} ); '))
        self.assertTrue(await client.query('is_nan( "3" ); '))
        self.assertTrue(await client.query('is_nan( set() ); '))

    async def test_is_nil(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_nil` takes 1 argument but 0 were given'):
            await client.query('is_nil();')

        self.assertTrue(await client.query('is_nil( nil ); '))
        self.assertFalse(await client.query('is_nil( 0 ); '))

    async def test_is_bytes(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_bytes` takes 1 argument but 0 were given'):
            await client.query('is_bytes();')

        self.assertFalse(await client.query('is_bytes( "pi" ); '))
        self.assertFalse(await client.query('is_bytes( "" ); '))
        self.assertFalse(await client.query('is_bytes( "ԉ" ); '))
        self.assertFalse(await client.query('is_bytes( [] );'))
        self.assertFalse(await client.query('is_bytes(nil);'))
        self.assertTrue(await client.query('is_bytes( bytes("pi") ); '))
        self.assertTrue(await client.query(
                'is_bytes(blob);',
                blob=pickle.dumps('binary')))

    async def test_is_closure(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_closure` takes 1 argument but 0 were given'):
            await client.query('is_closure();')

        self.assertTrue(await client.query('is_closure( ||nil ); '))
        self.assertFalse(await client.query('is_closure( "" ); '))

    async def test_is_raw(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_raw` takes 1 argument but 0 were given'):
            await client.query('is_raw();')

        self.assertTrue(await client.query('is_raw( "pi" ); '))
        self.assertTrue(await client.query('is_raw( "" ); '))
        self.assertTrue(await client.query('is_raw( "ԉ" ); '))
        self.assertFalse(await client.query('is_raw([]);'))
        self.assertFalse(await client.query('is_raw(nil);'))
        self.assertTrue(await client.query(
                'is_raw(blob);',
                blob=pickle.dumps('binary')))

    async def test_is_set(self, client):
        await client.query(r'.sa = set(); .sb = set([ {} ]);')
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_set` takes 1 argument but 0 were given'):
            await client.query('is_set();')

        self.assertFalse(await client.query(r'is_set({});'))
        self.assertFalse(await client.query('is_set([]);'))
        self.assertTrue(await client.query('is_set(set());'))
        self.assertTrue(await client.query('is_set(.sa);'))
        self.assertTrue(await client.query('is_set(.sb);'))

    async def test_is_str(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_str` takes 1 argument but 0 were given'):
            await client.query('is_str();')

        self.assertTrue(await client.query('is_str( "pi" ); '))
        self.assertTrue(await client.query('is_str( "" ); '))
        self.assertTrue(await client.query('is_str( "ԉ" ); '))
        self.assertFalse(await client.query('is_str([]);'))
        self.assertFalse(await client.query('is_str(nil);'))
        self.assertFalse(await client.query('is_str(123);'))
        self.assertFalse(await client.query(
                'is_str(blob);',
                blob=pickle.dumps('binary')))

    async def test_is_thing(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_thing` takes 1 argument but 0 were given'):
            await client.query('is_thing();')

        id = await client.query('.id();')

        self.assertTrue(await client.query(f'is_thing( #{id} ); '))
        self.assertTrue(await client.query(r'is_thing( {} ); '))
        self.assertTrue(await client.query(r'is_thing( thing(.id()) ); '))
        self.assertFalse(await client.query('is_thing( [] ); '))
        self.assertFalse(await client.query('is_thing( set() ); '))

    async def test_is_tuple(self, client):
        await client.query('.x = [[0, 1], nil]; .y = .x[0];')
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_tuple` takes 1 argument but 0 were given'):
            await client.query('is_tuple();')

        self.assertFalse(await client.query('is_tuple([]);'))
        self.assertFalse(await client.query('is_tuple(.x);'))
        self.assertFalse(await client.query('is_tuple(.y);'))
        self.assertTrue(await client.query('is_tuple(.x[0]);'))
        self.assertFalse(await client.query('is_tuple(0);'))
        self.assertFalse(await client.query('is_tuple("test");'))
        self.assertFalse(await client.query(r'is_tuple({});'))
        self.assertFalse(await client.query('is_tuple(.x[1]);'))

    async def test_is_utf8(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_utf8` takes 1 argument but 0 were given'):
            await client.query('is_utf8();')

        self.assertTrue(await client.query('is_utf8( "pi" ); '))
        self.assertTrue(await client.query('is_utf8( "" ); '))
        self.assertTrue(await client.query('is_utf8( "ԉ" ); '))
        self.assertFalse(await client.query('is_utf8([]);'))
        self.assertFalse(await client.query('is_utf8(nil);'))
        self.assertFalse(await client.query('is_utf8(123);'))
        self.assertFalse(await client.query(
                'is_utf8(blob);',
                blob=pickle.dumps('binary')))

    async def test_keys(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `keys`'):
            await client.query('nil.keys();')

        with self.assertRaisesRegex(
                LookupError,
                'function `keys` is undefined'):
            await client.query('keys();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `keys` takes 0 arguments but 1 was given'):
            await client.query('.keys(nil);')

        self.assertEqual(await client.query(r'{}.keys();'), [])
        self.assertEqual(await client.query(r'{a:1}.keys();'), ['a'])
        self.assertEqual(await client.query(r'{a:1, b:2}.keys();'), ['a', 'b'])

    async def test_len(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `len`'):
            await client.query('nil.len();')

        with self.assertRaisesRegex(
                NumArgumentsError,
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
                LookupError,
                'type `nil` has no function `lower`'):
            await client.query('nil.lower();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `lower` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".lower(nil);')

        self.assertEqual(await client.query('"".lower();'), "")
        self.assertEqual(await client.query('"l".lower();'), "l")
        self.assertEqual(await client.query('"HI !!".lower();'), "hi !!")

    async def test_each(self, client):
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
                LookupError,
                'type `nil` has no function `each`'):
            await client.query('nil.each(||nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `each` takes 1 argument but 0 were given'):
            await client.query('.each();')

        with self.assertRaisesRegex(
                TypeError,
                'function `each` expects argument 1 to be of type `closure` '
                'but got type `nil` instead'):
            await client.query('.each(nil);')

        self.assertEqual(await client.query(r'[].each(||1)'), None)
        self.assertEqual(await client.query(r'{}.each(||1)'), None)
        self.assertEqual(await client.query(r'set().each(||1)'), None)
        self.assertEqual(
            await client.query(r'range(10).each(|i| i+9999);'), None)

        self.assertEqual(
            set(await client.query('r = []; .iris.each(|k|r.push(k)); r;')),
            set({'name', 'age', 'likes'}))

        self.assertEqual(
            await client.query('r = []; range(3).each(|x| r.push(x)); r;'),
            [0, 1, 2])

        self.assertEqual(
            set(await client.query('r=[]; .girls.each(|g|r.push(g.name)); r')),
            set(['Iris', 'Cato'])
        )

        iris_id, cato_id = await client.query('[.iris.id(), .cato.id()];')

        self.assertEqual(
            set(await client.query('r=[]; .girls.each(|_,i| r.push(i)); r;')),
            set([iris_id, cato_id])
        )

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
                LookupError,
                'type `nil` has no function `map`'):
            await client.query('nil.map(||nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `map` takes 1 argument but 0 were given'):
            await client.query('.map();')

        with self.assertRaisesRegex(
                TypeError,
                'function `map` expects argument 1 to be of type `closure` '
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
                '.iris.map(|_, v| (is_array(v)) ? true:v);')),
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
                NumArgumentsError,
                'function `max_quota_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('max_quota_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `max_quota_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('max_quota_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `max_quota_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'max_quota_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('max_quota_err();')
        self.assertEqual(err['error_code'], Err.EX_MAX_QUOTA)
        self.assertEqual(err['error_msg'], "max quota is reached")

        err = await client.query('max_quota_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_MAX_QUOTA)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_node_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `node_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('node_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `node_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('node_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `node_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'node_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('node_err();')
        self.assertEqual(err['error_code'], Err.EX_NODE_ERROR)
        self.assertEqual(
            err['error_msg'],
            "node is temporary unable to handle the request")

        err = await client.query('node_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_NODE_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_now(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `now` takes 0 arguments but 2 were given'):
            await client.query('now(0, 1);')

        now = await client.query('now();')
        diff = time.time() - now
        self.assertGreater(diff, 0.0)
        self.assertLess(diff, 0.02)

    async def test_overflow_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `overflow_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('overflow_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `overflow_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('overflow_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `overflow_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'overflow_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('overflow_err();')
        self.assertEqual(err['error_code'], Err.EX_OVERFLOW)
        self.assertEqual(err['error_msg'], "integer overflow")

        err = await client.query('overflow_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_OVERFLOW)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_pop(self, client):
        await client.query('.list = [1, 2, 3];')
        self.assertEqual(await client.query('.list.pop()'), 3)
        self.assertEqual(await client.query('.list.pop()'), 2)
        self.assertEqual(await client.query('.list;'), [1])
        self.assertIs(await client.query('[nil].pop()'), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `pop`'):
            await client.query('nil.pop();')

        with self.assertRaisesRegex(
                LookupError,
                'type `tuple` has no function `pop`'):
            await client.query('.a = [.list]; .a[0].pop();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `pop` takes 0 arguments but 1 was given'):
            await client.query('.list.pop(nil);')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.pop());')

        with self.assertRaisesRegex(
                LookupError,
                'pop from empty list'):
            await client.query('[].pop();')

    async def test_shift(self, client):
        await client.query('.list = [1, 2, 3];')
        self.assertEqual(await client.query('.list.shift()'), 1)
        self.assertEqual(await client.query('.list.shift()'), 2)
        self.assertEqual(await client.query('.list;'), [3])
        self.assertIs(await client.query('[nil].shift()'), None)

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `shift`'):
            await client.query('nil.shift();')

        with self.assertRaisesRegex(
                LookupError,
                'type `tuple` has no function `shift`'):
            await client.query('.a = [.list]; .a[0].shift();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `shift` takes 0 arguments but 1 was given'):
            await client.query('.list.shift(nil);')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.shift());')

        with self.assertRaisesRegex(
                LookupError,
                'shift from empty list'):
            await client.query('[].shift();')

    async def test_push(self, client):
        await client.query('.list = [];')
        self.assertEqual(await client.query('.list.push("a")'), 1)
        self.assertEqual(await client.query('.list.push("b", "c")'), 3)
        self.assertEqual(await client.query('.list;'), ['a', 'b', 'c'])

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `push`'):
            await client.query('nil.push();')

        with self.assertRaisesRegex(
                LookupError,
                'type `tuple` has no function `push`'):
            await client.query('.a = [.list]; .a[0].push(nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `push` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.list.push();')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.push(4));')

    async def test_unshift(self, client):
        await client.query('.list = [];')
        self.assertEqual(await client.query('.list.unshift("c")'), 1)
        self.assertEqual(await client.query('.list.unshift("a", "b")'), 3)
        self.assertEqual(await client.query('.list;'), ['a', 'b', 'c'])

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `unshift`'):
            await client.query('nil.unshift();')

        with self.assertRaisesRegex(
                LookupError,
                'type `tuple` has no function `unshift`'):
            await client.query('.a = [.list]; .a[0].unshift(nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `unshift` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.list.unshift();')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.unshift(4));')

    async def test_raise(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `raise`'):
            await client.query('nil.raise();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `raise` takes at most 1 argument but 2 were given'):
            await client.query('raise(err(), 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `raise` expects argument 1 to be of type `error` '
                'or type `str` but got type `nil` instead'):
            await client.query('raise(nil);')

        with self.assertRaisesRegex(
                ThingsDBError,
                'error:-100'):
            await client.query('raise();')

        with self.assertRaisesRegex(
                ThingsDBError,
                'no licenses left'):
            await client.query('raise("no licenses left");')

        with self.assertRaisesRegex(
                ThingsDBError,
                'my custom error'):
            await client.query('raise(err(-100, "my custom error"));')

    async def test_rand(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `rand`'):
            await client.query('nil.rand();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `rand` takes 0 arguments but 1 was given'):
            await client.query('rand(3);')

        res = await client.query('rand();')
        self.assertTrue(isinstance(res, float))
        self.assertGreaterEqual(res, 0.0)
        self.assertLessEqual(res, 1.0)

    async def test_randint(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `randint`'):
            await client.query('nil.randint();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `randint` takes 2 arguments but 0 were given'):
            await client.query('randint();')

        with self.assertRaisesRegex(
                ValueError,
                'function `randint` does not accept an empty range'):
            await client.query('randint(10, 10);')

        with self.assertRaisesRegex(
                OverflowError,
                'integer overflow'):
            await client.query(
                'randint(-0x4000000000000000, 0x4000000000000001);')

        res = await client.query('randint(0, 10);')
        self.assertTrue(isinstance(res, int))
        self.assertGreaterEqual(res, 0)
        self.assertLess(res, 10)

        res = await client.query('randint(-15, -10);')
        self.assertTrue(isinstance(res, int))
        self.assertGreaterEqual(res, -15)
        self.assertLess(res, -10)

        values = set()
        for _ in range(200):
            values.add(await client.query('randint(5, 8);'))
        self.assertEqual(3, len(values))
        self.assertIn(5, values)
        self.assertIn(6, values)
        self.assertIn(7, values)

        res = await client.query(
                'randint(-0x4000000000000000, 0x4000000000000000);')
        self.assertTrue(isinstance(res, int))

    async def test_randstr(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `randstr`'):
            await client.query('nil.randstr();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `randstr` requires at least 1 argument '
                'but 0 were given'):
            await client.query('randstr();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `randstr` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('randstr(8, "abc", nil);')

        with self.assertRaisesRegex(
                ValueError,
                'function `randstr` requires a length between 0 and 1024'):
            await client.query('randstr(-1);')

        with self.assertRaisesRegex(
                ValueError,
                'function `randstr` requires a length between 0 and 1024'):
            await client.query('randstr(1025);')

        with self.assertRaisesRegex(
                ValueError,
                'function `randstr` requires a character set '
                'with at least one valid ASCII character'):
            await client.query('randstr(16, "");')

        with self.assertRaisesRegex(
                ValueError,
                'function `randstr` requires a character set '
                'with ASCII characters only'):
            await client.query('randstr(16, "abcԉpi");')

        res = await client.query('randstr(1024);')
        self.assertTrue(isinstance(res, str))
        self.assertEqual(len(res), 1024)

        res = await client.query('randstr(10, "abc");')
        for c in res:
            self.assertIn(c, "abc")

        self.assertEqual(await client.query('randstr(0);'), '')
        self.assertEqual(await client.query('randstr(3, "X");'), 'XXX')

    async def test_choice(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `set` has no function `choice`'):
            await client.query('set().choice();')

        with self.assertRaisesRegex(
                LookupError,
                'function `choice` is undefined'):
            await client.query('choice();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `choice` takes 0 arguments but 2 were given'):
            await client.query('[0].choice(1, 2);')

        with self.assertRaisesRegex(
                LookupError,
                'choice from empty list'):
            await client.query('[].choice();')

        res = await client.query('["a"].choice();')
        self.assertEqual(res, "a")

        values = set()
        for _ in range(200):
            values.add(await client.query('["a", "b", "c"].choice();'))
        self.assertEqual(3, len(values))
        self.assertIn("a", values)
        self.assertIn("b", values)
        self.assertIn('c', values)

    async def test_reduce(self, client):
        arr = [2, 3, 5, 7, 11]

        await client.query(r'''
            .list = arr;
        ''', arr=arr)

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `reduce`'):
            await client.query('nil.reduce(||nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `reduce` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.list.reduce();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `reduce` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('.list.reduce(|a, b| a+b, 2, 3);')

        with self.assertRaisesRegex(
                LookupError,
                'reduce on empty list with no initial value set'):
            await client.query('[].reduce(|a, b| a+b);')

        with self.assertRaisesRegex(
                LookupError,
                'reduce on empty set with no initial value set'):
            await client.query('set().reduce(|a, b| a+b);')

        res = await client.query(r'''
            .list.reduce(|a, b| a+b);
        ''')
        self.assertEqual(res, sum(arr))

        res = await client.query(r'''
            .list.reduce(|a, b| a+b, 42);
        ''')
        self.assertEqual(res, sum(arr) + 42)

        self.assertEqual(await client.query(r'[].reduce(||5, 42);'), 42)
        self.assertEqual(await client.query(r'[42].reduce(||5);'), 42)
        self.assertEqual(await client.query(r'[1,2,3].reduce(||42);'), 42)
        self.assertEqual(
            await client.query(r'[0,9,9].reduce(|a,_,i|a+i);'),
            0 + 1 + 2)
        self.assertEqual(await client.query(r'''
            set({x: 1}, {x: 2}, {x: 3}).reduce(|i, x| i + x.x, 0);
        '''), 6)
        self.assertEqual(await client.query(r'''
            set({x: 0}).reduce(|| 5);
        '''), {"x": 0})

        self.assertIs(await client.query(r'''
            set({x: 6}, {x: 7}).reduce(|i, x, id| id);
        '''), None)

    async def test_refs(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `refs`'):
            await client.query('"".refs();')

        with self.assertRaisesRegex(
                NumArgumentsError,
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
                LookupError,
                'type `nil` has no function `remove`'):
            await client.query('nil.remove(||true);')

        with self.assertRaisesRegex(
                LookupError,
                'type `tuple` has no function `remove`'):
            await client.query('.a = [.list]; .a[0].remove(||true);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `remove` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.list.remove();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `remove` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('.list.remove(||true, 2, 3);')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.list.map(||.list.remove(||true));')

        with self.assertRaisesRegex(
                TypeError,
                'function `remove` expects argument 1 to be of type `closure` '
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
                LookupError,
                'type `nil` has no function `remove`'):
            await client.query('nil.remove(||true);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `remove` requires at least 1 argument '
                'but 0 were given'):
            await client.query('.s.remove();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `remove` takes at most 1 argument '
                'when using a `closure` but 2 were given'):
            await client.query('.s.remove(||true, nil);')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `set` while the value is being used'):
            await client.query('.s.map(||.s.remove(||true));')

        with self.assertRaisesRegex(
                TypeError,
                'function `remove` expects argument 1 to be of type `closure` '
                'or type `thing` but got type `nil` instead'):
            await client.query('.s.remove(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `remove` expects argument 2 to be of type `thing` '
                'but got type `nil` instead'):
            await client.query('.s.remove(.t, nil);')

        # check if `t` is restored
        self.assertEqual(await client.query('.s.len();'), 3)

    async def test_range(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `range` takes at most 3 arguments '
                'but 4 were given'):
            await client.query('range(1, 2 ,3, 4);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `range` expects argument 1 to be of '
                r'type `int` but got type `float` instead'):
            await client.query('range(0.0);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `range` expects argument 2 to be of '
                r'type `int` but got type `nil` instead'):
            await client.query('range(0, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `range` expects argument 3 to be of '
                r'type `int` but got type `str` instead'):
            await client.query('range(0, 0, "1");')

        with self.assertRaisesRegex(
                ValueError,
                r'step value must not be zero'):
            await client.query('range(0, 0, 0);')

        with self.assertRaisesRegex(
                OperationError,
                r'maximum range length exceeded'):
            await client.query('range(0, 30000, 2);')

        self.assertEqual(
            await client.query('range(8)'), list(range(8)))
        self.assertEqual(
            await client.query('range(-8, 8)'), list(range(-8, 8)))
        self.assertEqual(
            await client.query('range(0, -8, -1)'), list(range(0, -8, -1)))
        for step in range(1, 12):
            self.assertEqual(
                await client.query('range(0, 10, step)', step=step),
                list(range(0, 10, step)))

    async def test_return(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `return` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('return(1, 2 ,3);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `return` expects argument 2 to be of '
                r'type `int` but got type `float` instead'):
            await client.query('return(nil, 0.0);')

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a `deep` value between 0 and 127 '
                r'but got 200 instead'):
            await client.query('return(nil, 200);')

        with self.assertRaisesRegex(
                ValueError,
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
                LookupError,
                'type `nil` has no function `set`'):
            await client.query('nil.set();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `set` takes 2 arguments but 0 were given'):
            await client.query('.set();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `set` expects argument 1 to be of '
                r'type `str` but got type `nil` instead'):
            await client.query('.set(nil, nil);')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `thing` while the value is being used'):
            await client.query('.map(||.set("a", 42));')

        with self.assertRaisesRegex(
                OperationError,
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
                LookupError,
                'type `nil` has no function `set`'):
            await client.query('nil.set();')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `nil` to `set`'):
            await client.query('set(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot add type `int` to a set'):
            await client.query(r'set([{}, thing(.id()), 3]);')

        self.assertEqual(await client.query('set();'), [])
        self.assertEqual(await client.query('set([]);'), [])
        self.assertEqual(await client.query('set(set());'), [])
        self.assertEqual(
            await client.query(r'set([{}, {}]);'),
            [{}, {}])
        self.assertEqual(
            await client.query(r'set({}, {});'),
            [{}, {}])
        self.assertEqual(
            await client.query(r'set({});'),
            [{}])

    async def test_every(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'function `every` is undefined'):
            await client.query('every(||true);')

        with self.assertRaisesRegex(
                LookupError,
                'type `int` has no function `every`'):
            await client.query('(1).every(||true);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `every` takes 1 argument but 0 were given'):
            await client.query('[].every();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `every` expects argument 1 to be of type `closure` '
                r'but got type `int` instead'):
            await client.query('[1,2,3].every(42);')

        with self.assertRaisesRegex(
                TypeError,
                r'`\+` not supported between `int` and `str`'):
            await client.query('[1,2,3].every(|x| x + "text");')

        self.assertFalse(await client.query('[1, 2, 3].every(|x| x > 2)'))
        self.assertTrue(await client.query('[1, 2, 3].every(|x| x <= 3)'))
        self.assertTrue(await client.query('[].every(||false)'))
        self.assertFalse(await client.query(r'''
            set({x: 1}, {x: 2}, {x: 3}).every(|x| x.x > 2)
        '''))
        self.assertTrue(await client.query(r'''
            set({x: 1}, {x: 2}, {x: 3}).every(|x| x.x <= 3)
        '''))
        self.assertTrue(await client.query('set().every(||false)'))

    async def test_some(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'function `some` is undefined'):
            await client.query('some(||true);')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `some`'):
            await client.query('nil.some(||true);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `some` takes 1 argument but 0 were given'):
            await client.query('[].some();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `some` expects argument 1 to be of type `closure` '
                r'but got type `str` instead'):
            await client.query('[1,2,3].some("text");')

        self.assertTrue(await client.query('[1, 2, 3].some(|x| x > 2)'))
        self.assertFalse(await client.query('[1, 2, 3].some(|x| x > 3)'))
        self.assertFalse(await client.query('[].some(||true)'))
        self.assertTrue(await client.query(r'''
            set({x: 1}, {x: 2}, {x: 3}).some(|x| x.x > 2)
        '''))
        self.assertFalse(await client.query(r'''
             set({x: 1}, {x: 2}, {x: 3}).some(|x| x.x > 3)
        '''))
        self.assertFalse(await client.query('set().some(||true)'))

    async def test_sort(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `sort`'):
            await client.query('nil.sort();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `sort` takes at most 2 arguments but 3 were given'):
            await client.query('[2, 0, 1, 3].sort(|a, b|1, nil, nil);')

        with self.assertRaisesRegex(
                OperationError,
                'function `sort` cannot be used recursively'):
            await client.query('[2, 0, 1, 3].sort(|a, b| [].sort());')

        with self.assertRaisesRegex(
                TypeError,
                'function `sort` expects argument 1 to be of type `closure` '
                'but got type `nil` instead'):
            await client.query('[2, 0, 1, 3].sort(nil);')

        with self.assertRaisesRegex(
                TypeError,
                '`<` not supported between `str` and `int`'):
            await client.query('["a", 1].sort();')

        with self.assertRaisesRegex(
                TypeError,
                'expecting a return value of type `int` but '
                'got type `bool` instead'):
            await client.query('["a", "b"].sort(|a, b| true);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `sort` requires a closure which accepts '
                '1 or 2 arguments'):
            await client.query('[2, 0, 1, 3].sort(||1);')

        self.assertEqual(await client.query(r'''
            [2, 0, 1, 3].sort();
        '''), [0, 1, 2, 3])

        self.assertEqual(await client.query(r'''
            [2, 0, 1, 3].sort(true);
        '''), [3, 2, 1, 0])

        self.assertEqual(await client.query(r'''
            ["iris", "anne", "cato"].sort();
        '''), ["anne", "cato", "iris"])

        self.assertEqual(await client.query(r'''
            ["Iris", "Anne", "cato"].sort(|n| n.lower());
        '''), ["Anne", "cato", "Iris"])

        self.assertEqual(await client.query(r'''
            [42, 2013, 6].sort(|a, b| a > b ? -1 : a < b ? 1 : 0);
        '''), [2013, 42, 6])

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
                LookupError,
                'type `bool` has no function `splice`'):
            await client.query('true.splice();')

        with self.assertRaisesRegex(
                LookupError,
                'type `tuple` has no function `splice`'):
            await client.query('a = [.li]; a[0].splice(nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `splice` requires at least 2 arguments '
                'but 0 were given'):
            await client.query('.li.splice();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `splice` requires at least 2 arguments '
                'but 1 was given'):
            await client.query('.li.splice(0);')

        with self.assertRaisesRegex(
                OperationError,
                r'cannot change type `list` while the value is being used'):
            await client.query('.li.map(||.li.splice(0, 1));')

        with self.assertRaisesRegex(
                TypeError,
                r'function `splice` expects argument 1 to be of '
                r'type `int` but got type `float` instead'):
            await client.query('.li.splice(0.0, 0)')

        with self.assertRaisesRegex(
                TypeError,
                r'function `splice` expects argument 2 to be of '
                r'type `int` but got type `nil` instead'):
            await client.query('.li.splice(0, nil)')

    async def test_starts_with(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `starts_with`'):
            await client.query('nil.starts_with("world!");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `starts_with` takes 1 argument '
                'but 2 were given'):
            await client.query('"Hi World!".starts_with("x", 2)')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `starts_with` takes 1 argument '
                'but 0 were given'):
            await client.query('"Hi World!".starts_with()')

        with self.assertRaisesRegex(
                TypeError,
                r'function `starts_with` expects argument 1 to be of '
                r'type `str` but got type `list` instead'):
            await client.query('"Hi World!".starts_with([]);')

        self.assertTrue(await client.query('"Hi World!".starts_with("")'))
        self.assertFalse(await client.query('"".starts_with("!")'))
        self.assertTrue(await client.query('"Hi World!".starts_with("Hi")'))
        self.assertFalse(await client.query('"Hi World!".starts_with("hi")'))

    async def test_str(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `str`'):
            await client.query('nil.str();')

        with self.assertRaisesRegex(
                NumArgumentsError,
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

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `list` to `str`'):
            await client.query('str([]);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `thing` to `str`'):
            await client.query('str({});')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `closure` to `str`'):
            await client.query('str(||nil);')

    async def test_syntax_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `syntax_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('syntax_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `syntax_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('syntax_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `syntax_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'syntax_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('syntax_err();')
        self.assertEqual(err['error_code'], Err.EX_SYNTAX_ERROR)
        self.assertEqual(err['error_msg'], "syntax error in query")

        err = await client.query('syntax_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_SYNTAX_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_test(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `regex` has no function `test`'):
            await client.query('(/.*/).test();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `test` takes 1 argument but 0 were given'):
            await client.query('"".test();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `test` expects argument 1 to be of '
                r'type `regex` but got type `str` instead'):
            await client.query('"".test("abc");')

        self.assertTrue(await client.query(r'"".test(/.*/);'))
        self.assertTrue(await client.query(r'"Hi".test(/hi/i);'))
        self.assertTrue(await client.query(r'"hello!".test(/hello.*/);'))
        self.assertFalse(await client.query(r'"Hi".test(/hi/);'))
        self.assertFalse(await client.query(r'"hello".test(/hello!.*/);'))

    async def test_thing(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `thing`'):
            await client.query('nil.thing();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `thing` takes at most 1 argument but 2 were given'):
            await client.query('thing(1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `regex` to `thing`'):
            await client.query('thing(/.*/);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `nil` to `thing`'):
            await client.query('thing(nil);')

        with self.assertRaisesRegex(
                LookupError,
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
                NumArgumentsError,
                'function `try` requires at least 1 argument '
                'but 0 were given'):
            await client.query('try();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `try` expects arguments 2..X to be of '
                r'type `error` but got type `nil` instead'):
            await client.query('try((10 / 0), nil);')

        self.assertIs(await client.query('is_err(try( (10 / 2) ));'), False)
        self.assertIs(await client.query('is_err(try( (10 / 0) ));'), True)
        self.assertIs(await client.query(
            'is_err(try( (10 / 0), zero_div_err() ));'), True)

        with self.assertRaisesRegex(
                ZeroDivisionError,
                'division or modulo by zero'):
            await client.query('try( (10 / 0), lookup_err());')

    async def test_type(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `type`'):
            await client.query('"".type();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `type` takes 1 argument but 0 were given'):
            await client.query('type();')

        self.assertEqual(await client.query('type(nil);'), "nil")
        self.assertEqual(await client.query('type(true);'), "bool")
        self.assertEqual(await client.query('type(1);'), "int")
        self.assertEqual(await client.query('type(0.0);'), "float")
        self.assertEqual(await client.query('type("Hi");'), "str")
        self.assertEqual(await client.query('type(bytes("Hi"));'), "bytes")
        self.assertEqual(await client.query('type([]);'), "list")
        self.assertEqual(await client.query('type([[]][0]);'), "tuple")
        self.assertEqual(await client.query(r'type({});'), "thing")
        self.assertEqual(await client.query('type(set());'), "set")
        self.assertEqual(await client.query('type(||nil);'), "closure")
        self.assertEqual(await client.query('type(/.*/);'), "regex")

    async def test_upper(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `upper`'):
            await client.query('nil.upper();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `upper` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".upper(nil);')

        self.assertEqual(await client.query('"".upper();'), "")
        self.assertEqual(await client.query('"U".upper();'), "U")
        self.assertEqual(await client.query('"hi !!".upper();'), "HI !!")

    async def test_trim(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `trim`'):
            await client.query('nil.trim();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `trim` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".trim(nil);')

        self.assertEqual(await client.query('"".trim();'), "")
        self.assertEqual(await client.query('"U".trim();'), "U")
        self.assertEqual(await client.query(r'''
            "
              hi  !!
            ".trim();
        '''), "hi  !!")
        self.assertEqual(await client.query(r'''
            "hi  !!
            ".trim();
        '''), "hi  !!")
        self.assertEqual(await client.query(r'''
            "
              hi  !!".trim();
        '''), "hi  !!")

        await client.query(r'''
            x = "TestString";
            b = refs(x);
            y = x.trim();
            assert (refs(x) == b + 1 );
        ''')

    async def test_trim_left(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `trim_left`'):
            await client.query('nil.trim_left();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `trim_left` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".trim_left(nil);')

        self.assertEqual(await client.query('"".trim_left();'), "")
        self.assertEqual(await client.query('"U".trim_left();'), "U")
        self.assertEqual(await client.query(r'''
            "
              hi  !!   ".trim_left();
        '''), "hi  !!   ")

        await client.query(r'''
            x = "TestString   ";
            b = refs(x);
            y = x.trim_left();
            assert (refs(x) == b + 1 );
        ''')

    async def test_trim_right(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `trim_right`'):
            await client.query('nil.trim_right();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `trim_right` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".trim_right(nil);')

        self.assertEqual(await client.query('"".trim_right();'), "")
        self.assertEqual(await client.query('"U".trim_right();'), "U")
        self.assertEqual(await client.query(r'''
            "   hi  !!

            ".trim_right();
        '''), "   hi  !!")

        await client.query(r'''
            x = "   TestString";
            b = refs(x);
            y = x.trim_right();
            assert (refs(x) == b + 1 );
        ''')

    async def test_join(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `str` has no function `join`'):
            await client.query('"".join();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `join` takes at most 1 argument '
                'but 3 were given'):
            await client.query('["Hello", "World"].join(" ", nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `join` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query('["Hello", "World"].join(bytes(" "));')

        with self.assertRaisesRegex(
                TypeError,
                'expecting item 0 to be of type `str` '
                'but got type `int` instead'):
            await client.query('[0, "a"].join();')

        with self.assertRaisesRegex(
                TypeError,
                'expecting item 2 to be of type `str` '
                'but got type `bytes` instead'):
            await client.query('["a", "b", bytes("c")].join();')

        self.assertEqual(await client.query(r'''
            [
                [].join(),
                [].join(""),
                ["a"].join(),
                ["a"].join(''),
                ["a"].join(" - "),
                ["a", "b"].join(),
                ["a", "b"].join(''),
                ["a", "b"].join(' - '),
                ["a", "b", "c"].join(),
                ["a", "b", "c"].join(''),
                ["a", "b", "c"].join(" - "),
                ["Hello", "World"].join(" "),
                range(5).map(|i| str(i)).join(' '),
            ]
        '''), [
            '',
            '',
            'a',
            'a',
            'a',
            'ab',
            'ab',
            'a - b',
            'abc',
            'abc',
            'a - b - c',
            'Hello World',
            '0 1 2 3 4'
        ])

    async def test_split(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `split`'):
            await client.query('nil.split();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `split` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('"Hello World".split("x", 0, nil);')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `split` takes at most 1 argument '
                'when the first argument is of type `int`'):
            await client.query('"Hello World".split(1, "a");')

        with self.assertRaisesRegex(
                TypeError,
                'function `split` expects argument 1 to be of '
                'type `str` or type `int` but got type `nil` instead'):
            await client.query('"Hello World".split(nil);')

        with self.assertRaisesRegex(
                ValueError,
                'empty separator'):
            await client.query('"Hello World".split("");')

        with self.assertRaisesRegex(
                OverflowError,
                'integer overflow'):
            await client.query('"".split(-0x7fffffffffffffff - 1);')

        self.assertEqual(await client.query(r'''
            [
                "".split(),
                " ".split(),
                "  ".split(),
                "My
                name
                is
                Iris".split(),
                "Hello World!".split(),
                "a,b,c".split(','),
                "a, b, c".split(', '),
                'ttttt'.split('tt'),
                "This  is  a  test!".split(2),
                "comma,separate,this,line!".split(',', 0),
                "comma,separate,this,line!".split(',', 1),
                "comma,separate,this,line!".split(',', 2),
                "comma,separate,this,line!".split(',', 3),
                "comma,separate,this,line!".split(',', 99),
                "comma,separate,this,line!".split(',', -1),
                "comma,separate,this,line!".split(',', -2),
                "comma,separate,this,line!".split(',', -3),
                "comma,separate,this,line!".split(',', -99),
                "".split(-1),
                " ".split(-1),
                "This  is  a  test!".split(-2),
                "This  is  a  test!".split(-0),
            ]
        '''), [
            [''],
            ['', ''],
            ['', ''],
            ['My', 'name', 'is', 'Iris'],
            ['Hello', 'World!'],
            ['a', 'b', 'c'],
            ['a', 'b', 'c'],
            ['', '', 't'],
            ['This', 'is', 'a  test!'],
            ['comma,separate,this,line!'],
            ['comma', 'separate,this,line!'],
            ['comma', 'separate', 'this,line!'],
            ['comma', 'separate', 'this', 'line!'],
            ['comma', 'separate', 'this', 'line!'],
            ['comma,separate,this', 'line!'],
            ['comma,separate', 'this', 'line!'],
            ['comma', 'separate', 'this', 'line!'],
            ['comma', 'separate', 'this', 'line!'],
            [''],
            ['', ''],
            ['This  is', 'a', 'test!'],
            ['This  is  a  test!'],

        ])

    async def test_replace(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `replace`'):
            await client.query('nil.replace();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `replace` requires at least 2 arguments '
                'but 1 was given'):
            await client.query('"Hello World".replace("x");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `replace` takes at most 3 arguments '
                'but 4 were given'):
            await client.query('"Hello World".replace("x", "y", 0, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `replace` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('"Hello World".replace(nil, "y");')

        with self.assertRaisesRegex(
                TypeError,
                'function `replace` expects argument 2 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('"Hello World".replace("x", nil);')

        with self.assertRaisesRegex(
                ValueError,
                'replace an empty string'):
            await client.query('"Hello World".replace("", "y");')

        with self.assertRaisesRegex(
                OverflowError,
                'integer overflow'):
            await client.query('"".replace("a", "", -0x7fffffffffffffff - 1);')

        self.assertEqual(await client.query(r'''
            [
                "a,b,c".replace(',', '-'),
                ",,,".replace(',', '-'),
                "".replace('aa', 'a'),
                "a".replace('aa', 'a'),
                "aa".replace('aa', 'a'),
                "aaa".replace('aa', 'a'),
                "".replace('a', 'aa'),
                "a".replace('a', 'aa'),
                "aa".replace('a', 'aa'),
                "This is a test!".replace(' is ', ' was '),
                "aaa".replace('a', 'x', 0),
                "aaa".replace('a', 'x', 1),
                "aaa".replace('a', 'x', 2),
                "aaa".replace('a', 'x', 3),
                "aaa".replace('a', 'x', 4),
                "aaa".replace('a', 'x', -1),
                "aaa".replace('a', 'x', -2),
                "aaa".replace('a', 'x', -3),
                "aaa".replace('a', 'x', -4),
                "xxx".replace('y', 'z'),
                "xxx".replace('y', 'z', 0),
                "xxx".replace('y', 'z', 1),
                "xxx".replace('y', 'z', -1),
                "This is a test!".replace('is', 'was', -1),
                "Hi!".replace('Hi', 'Hello'),
                "Hi".replace('Hi', 'Hello'),
                "Hi".replace('Hi', 'Hello', 99),
                "Hi".replace('Hi', 'Hello', -99),
            ]
        '''), [
            'a-b-c',
            '---',
            '',
            'a',
            'a',
            'aa',
            '',
            'aa',
            'aaaa',
            'This was a test!',
            'aaa',
            'xaa',
            'xxa',
            'xxx',
            'xxx',
            'aax',
            'axx',
            'xxx',
            'xxx',
            'xxx',
            'xxx',
            'xxx',
            'xxx',
            'This was a test!',
            'Hello!',
            'Hello',
            'Hello',
            'Hello',
        ])

    async def test_values(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `values`'):
            await client.query('nil.values();')

        with self.assertRaisesRegex(
                LookupError,
                'function `values` is undefined'):
            await client.query('values();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `values` takes 0 arguments but 1 was given'):
            await client.query('.values(nil);')

        self.assertEqual(await client.query(r'{}.values();'), [])
        self.assertEqual(await client.query(r'{a:1}.values();'), [1])
        self.assertEqual(await client.query(r'{a:1, b:2}.values();'), [1, 2])

    async def test_watch(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `watch`'):
            await client.query('nil.watch();')

        with self.assertRaisesRegex(
                LookupError,
                'function `watch` is undefined'):
            await client.query('watch();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `watch` takes 0 arguments but 1 was given'):
            await client.query('.watch(nil);')

        with self.assertRaisesRegex(
                ValueError,
                'thing has no `#ID`; '
                'if you really want to watch this `thing` then you need to '
                'assign it to a collection;'):
            await client.query('{}.watch();')

        self.assertIs(await client.query(r'.watch();'), None)

    async def test_unwatch(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `unwatch`'):
            await client.query('nil.unwatch();')

        with self.assertRaisesRegex(
                LookupError,
                'function `unwatch` is undefined'):
            await client.query('unwatch();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `unwatch` takes 0 arguments but 1 was given'):
            await client.query('.unwatch(nil);')

        self.assertIs(await client.query(r'{}.unwatch();'), None)
        self.assertIs(await client.query(r'.unwatch();'), None)

    async def test_wse(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `wse`'):
            await client.query('nil.wse();')

        with self.assertRaisesRegex(
                NumArgumentsError,
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
                NumArgumentsError,
                'function `zero_div_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('zero_div_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `zero_div_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('zero_div_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `zero_div_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'zero_div_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('zero_div_err();')
        self.assertEqual(err['error_code'], Err.EX_ZERO_DIV)
        self.assertEqual(err['error_msg'], "division or module by zero")

        err = await client.query('zero_div_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_ZERO_DIV)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_cancelled_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `cancelled_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('cancelled_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `cancelled_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('cancelled_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `cancelled_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'cancelled_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('cancelled_err();')
        self.assertEqual(err['error_code'], Err.EX_CANCELLED)
        self.assertEqual(
            err['error_msg'],
            "operation is cancelled before completion")

        err = await client.query('cancelled_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_CANCELLED)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_value_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `value_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('value_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `value_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('value_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `value_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'value_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('value_err();')
        self.assertEqual(err['error_code'], Err.EX_VALUE_ERROR)
        self.assertEqual(
            err['error_msg'],
            "object has the right type but an inappropriate value")

        err = await client.query('value_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_VALUE_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_type_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `type_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('type_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `type_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('type_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `type_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'type_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('type_err();')
        self.assertEqual(err['error_code'], Err.EX_TYPE_ERROR)
        self.assertEqual(err['error_msg'], "object of inappropriate type")

        err = await client.query('type_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_TYPE_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_num_arguments_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `num_arguments_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('num_arguments_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `num_arguments_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('num_arguments_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `num_arguments_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'num_arguments_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('num_arguments_err();')
        self.assertEqual(err['error_code'], Err.EX_NUM_ARGUMENTS)
        self.assertEqual(err['error_msg'], "wrong number of arguments")

        err = await client.query('num_arguments_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_NUM_ARGUMENTS)
        self.assertEqual(err['error_msg'], "my custom error msg")

    async def test_operation_err(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `operation_err` takes at most 1 argument '
                'but 2 were given'):
            await client.query('operation_err("bla", 2);')

        with self.assertRaisesRegex(
                TypeError,
                'function `operation_err` expects argument 1 to be of '
                'type `str` but got type `nil` instead'):
            await client.query('operation_err(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `operation_err` expects argument 1 to be of '
                'type `str` but got type `bytes` instead'):
            await client.query(
                'operation_err(blob);',
                blob=pickle.dumps({}))

        err = await client.query('operation_err();')
        self.assertEqual(err['error_code'], Err.EX_OPERATION_ERROR)
        self.assertEqual(
            err['error_msg'],
            "operation is not valid in the current context")

        err = await client.query('operation_err("my custom error msg");')
        self.assertEqual(err['error_code'], Err.EX_OPERATION_ERROR)
        self.assertEqual(err['error_msg'], "my custom error msg")


if __name__ == '__main__':
    run_test(TestCollectionFunctions())
