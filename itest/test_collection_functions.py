#!/usr/bin/env python
import asyncio
import pickle
import time
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import BadRequestError
from thingsdb.exceptions import IndexError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError


class TestCollectionFunctions(TestBase):

    title = 'Test collection scope functions'

    @default_test_setup(num_nodes=2, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.run_tests(client)

        # add another node so away node and gc is forced
        await self.node1.join_until_ready(client)

        # expected no garbage collection
        counters = await client.query('counters();', target=client.node)
        self.assertEqual(counters['garbage_collected'], 0)

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

        self.assertEqual(await client.query('assert((2>1)); 42;'), 42)

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` requires at least 1 argument '
                'but 0 were given'):
            await client.query('assert();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` takes at most 3 arguments '
                'but 4 were given'):
            await client.query('assert(true, "x", 1, nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` expects argument 2 to be of '
                'type `raw` but got type `nil` instead'):
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
                'type `int` but got type `nil` instead'):
            await client.query('assert(false, "msg", nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` expects a custom error_code between '
                '1 and 32 but got 0 instead'):
            await client.query('assert(false, "msg", 0);')

    async def test_blob(self, client):
        self.assertIs(await client.query(
            'objects = [blob(0), blob(1)]; nil;',
            blobs=(pickle.dumps({}), pickle.dumps([]))), None)

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
                'type `int` but got type `nil` instead'):
            await client.query('blob(nil);')

        with self.assertRaisesRegex(
                IndexError, 'blob index out of range'):
            await client.query('blob(0);')

    async def test_del(self, client):
        await client.query(r'greet = "Hello world";')

        with self.assertRaisesRegex(
                IndexError,
                'type `raw` has no function `del`'):
            await client.query('greet.del("x");')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `del` takes 1 argument '
                'but 0 were given'):
            await client.query('del();')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `del` can only be used on things with an id > 0; '
                r'\(things which are assigned automatically receive an id\)'):
            await client.query('{x:1}.del("x");')

        with self.assertRaisesRegex(
                BadRequestError,
                r'cannot use `del` while thing `#\d+` is in use'):
            await client.query('map( ||del("greet") );')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `del` expects argument 1 to be of type `raw` '
                r'but got type `nil` instead'):
            await client.query('del(nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `del` expects argument 1 to be a valid name.*'):
            await client.query('del("");')

        with self.assertRaisesRegex(
                IndexError,
                r'thing `#\d+` has no property `x`'):
            await client.query(' del("x");')

        self.assertIs(await client.query(r'del("greet");'), None)
        self.assertFalse(await client.query(r'hasprop("greet");'))

    async def test_endswith(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `endswith`'):
            await client.query('nil.endswith("world!");')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `endswith` takes 1 argument but 2 were given'):
            await client.query('"Hi World!".endswith("x", 2)')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `endswith` expects argument 1 to be of '
                r'type `raw` but got type `int` instead'):
            await client.query('"Hi World!".endswith(1);')

        self.assertTrue(await client.query('"Hi World!".endswith("")'))
        self.assertFalse(await client.query('"".endswith("!")'))
        self.assertTrue(await client.query('"Hi World!".endswith("World!")'))
        self.assertFalse(await client.query('"Hi World!".endswith("world!")'))

    async def test_filter(self, client):
        await client.query(r'''
            iris = {
                name: 'Iris',
                age: 6,
                likes: ['k3', 'swimming', 'red', 6],
            };
        ''')
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `filter`'):
            await client.query('nil.filter(||true);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `filter` takes 1 argument but 0 were given'):
            await client.query('filter();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `filter` expects argument 1 to be a `closure` '
                'but got type `nil` instead'):
            await client.query('filter(nil);')

        self.assertEqual(
            (await client.query('iris.filter(|k|(k == "name"));')),
            {'#': 0, 'name': 'Iris'})

        self.assertEqual(
            (await client.query('iris.filter(|_k, v|(v == 6));')),
            {'#': 0, 'age': 6})

        self.assertEqual(
            (await client.query('iris.likes.filter(|v|isstr(v));')),
            ['k3', 'swimming', 'red'])

        self.assertEqual(
            (await client.query('iris.likes.filter(|_v, i|(i > 1));')),
            ['red', 6])

        self.assertEqual(await client.query('iris.filter(||nil);'), {'#': 0})
        self.assertEqual(await client.query('iris.likes.filter(||nil);'), [])
        self.assertEqual(await client.query(r'{}.filter(||true)'), {'#': 0})
        self.assertEqual(await client.query(r'[].filter(||true)'), [])

    async def test_find(self, client):
        await client.query(r'x = [42, "gallaxy"];')

        with self.assertRaisesRegex(
                IndexError,
                'type `thing` has no function `find`'):
            await client.query('find(||true);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `find` requires at least 1 argument '
                'but 0 were given'):
            await client.query('x.find();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `find` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('x.find(||true, 2, 3);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `find` expects argument 1 to be a `closure` '
                'but got type `int` instead'):
            await client.query('x.find(0);')

        self.assertIs(await client.query('[].find(||true);'), None)
        self.assertIs(await client.query('[].find(||false);'), None)
        self.assertEqual(await client.query('x.find(||true);'), 42)
        self.assertEqual(await client.query('x.find(|v|(v==42));'), 42)
        self.assertEqual(await client.query('x.find(|_,i|(i>=0));'), 42)
        self.assertEqual(await client.query('x.find(|_,i|(i>=1));'), "gallaxy")
        self.assertIs(await client.query('x.find(||nil);'), None)
        self.assertEqual(await client.query('x.find(||nil, 42);'), 42)

    async def test_findindex(self, client):
        await client.query(r'x = [42, ""];')

        with self.assertRaisesRegex(
                IndexError,
                'type `thing` has no function `findindex`'):
            await client.query('findindex(||true);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `findindex` takes 1 argument but 2 were given'):
            await client.query('x.findindex(0, 1);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `findindex` expects argument 1 to be a `closure` '
                'but got type `bool` instead'):
            await client.query('x.findindex(true);')

        self.assertIs(await client.query('[].findindex(||true);'), None)
        self.assertIs(await client.query('[].findindex(||false);'), None)
        self.assertEqual(await client.query('x.findindex(||true);'), 0)
        self.assertIs(await client.query('x.findindex(||false);'), None)
        self.assertEqual(await client.query('x.findindex(|_,i|(i>0));'), 1)
        self.assertEqual(await client.query('x.findindex(|v|(v==42));'), 0)
        self.assertEqual(await client.query('x.findindex(|v|isstr(v));'), 1)

    async def test_hasprop(self, client):
        await client.query(r'x = 0.0;')

        with self.assertRaisesRegex(
                IndexError,
                'type `float` has no function `hasprop`'):
            await client.query('x.hasprop("x");')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `hasprop` takes 1 argument but 0 were given'):
            await client.query('hasprop();')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `hasprop` expects argument 1 to be of '
                r'type `raw` but got type `int` instead'):
            await client.query('hasprop(id());')

        self.assertTrue(await client.query('hasprop("x");'))
        self.assertFalse(await client.query('hasprop("y");'))

    async def test_id(self, client):
        o = await client.query(r'o = {};')
        self.assertTrue(isinstance(o['#'], int))
        self.assertGreater(o['#'], 0)
        self.assertEqual(await client.query(r'o.id();'), o['#'])
        self.assertEqual(await client.query(r'{}.id();'), 0)

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `id`'):
            await client.query('nil.id();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `id` takes 0 arguments but 1 was given'):
            await client.query('id(nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `id` takes 0 arguments but 2 were given'):
            await client.query('id(nil, nil);')

    async def test_indexof(self, client):
        await client.query(r'x = [42, "thingsdb", t(id()), 42, false, nil];')

        with self.assertRaisesRegex(
                IndexError,
                'type `thing` has no function `indexof`'):
            await client.query('indexof("x");')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `indexof` takes 1 argument but 0 were given'):
            await client.query('x.indexof();')

        self.assertEqual(await client.query('x.indexof(42);'), 0)
        self.assertEqual(await client.query('x.indexof("thingsdb");'), 1)
        self.assertEqual(await client.query('x.indexof(t(id()));'), 2)
        self.assertEqual(await client.query('x.indexof(false);'), 4)
        self.assertEqual(await client.query('x.indexof(nil);'), 5)
        self.assertIs(await client.query('x.indexof(42.1);'), None)
        self.assertIs(await client.query('x.indexof("ThingsDb");'), None)
        self.assertIs(await client.query('x.indexof(true);'), None)
        self.assertIs(await client.query(r'x.indexof({});'), None)

        # cleanup garbage, the reference to the collection
        await client.query(r'''x.splice(2, 1);''')

    async def test_int(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `int`'):
            await client.query('nil.int();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `int` takes 1 argument but 0 were given'):
            await client.query('int();')

        with self.assertRaisesRegex(
                BadRequestError,
                'cannot convert type `regex` to `int`'):
            await client.query('int(/.*/);')

        with self.assertRaisesRegex(
                BadRequestError,
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
        self.assertEqual(await client.query('int(3.14);'), 3)
        self.assertEqual(await client.query('int(-3.14);'), -3)
        self.assertEqual(await client.query('int(42.9);'), 42)
        self.assertEqual(await client.query('int(-42.9);'), -42)
        self.assertEqual(await client.query('int(true);'), 1)
        self.assertEqual(await client.query('int(false);'), 0)
        self.assertEqual(await client.query('int("3.14");'), 3)
        self.assertEqual(await client.query('int("-3.14");'), -3)

    async def test_isarray(self, client):
        await client.query('x = [[0, 1], nil]; y = x[0];')
        with self.assertRaisesRegex(
                BadRequestError,
                'function `isarray` takes 1 argument but 0 were given'):
            await client.query('isarray();')

        self.assertTrue(await client.query('isarray([]);'))
        self.assertTrue(await client.query('isarray(x);'))
        self.assertTrue(await client.query('isarray(y);'))
        self.assertTrue(await client.query('isarray(x[0]);'))
        self.assertFalse(await client.query('isarray(0);'))
        self.assertFalse(await client.query('isarray("test");'))
        self.assertFalse(await client.query(r'isarray({});'))
        self.assertFalse(await client.query('isarray(x[1]);'))

    async def test_isascii(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
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
                BadRequestError,
                'function `isbool` takes 1 argument but 3 were given'):
            await client.query('isbool(1, 2, 3);')

        self.assertTrue(await client.query('isbool( true ); '))
        self.assertTrue(await client.query('isbool( false ); '))
        self.assertTrue(await client.query('isbool( isint(id()) ); '))
        self.assertFalse(await client.query('isbool( "ԉ" ); '))
        self.assertFalse(await client.query('isbool([]);'))
        self.assertFalse(await client.query('isbool(nil);'))

    async def test_isfloat(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
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
                BadRequestError,
                'function `isinf` takes 1 argument but 0 were given'):
            await client.query('isinf();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `isinf` expects argument 1 to be of '
                'type `float` but got type `nil` instead'):
            await client.query('isinf(nil);')

        self.assertTrue(await client.query('isinf( inf ); '))
        self.assertTrue(await client.query('isinf( -inf ); '))
        self.assertFalse(await client.query('isinf( 0.0 ); '))
        self.assertFalse(await client.query('isinf( nan ); '))
        self.assertFalse(await client.query('isinf( 42 ); '))
        self.assertFalse(await client.query('isinf( true ); '))

    async def test_islist(self, client):
        await client.query('x = [[0, 1], nil]; y = x[0];')
        with self.assertRaisesRegex(
                BadRequestError,
                'function `islist` takes 1 argument but 0 were given'):
            await client.query('islist();')

        self.assertTrue(await client.query('islist([]);'))
        self.assertTrue(await client.query('islist(x);'))
        self.assertTrue(await client.query('islist(y);'))
        self.assertFalse(await client.query('islist(x[0]);'))
        self.assertFalse(await client.query('islist(0);'))
        self.assertFalse(await client.query('islist("test");'))
        self.assertFalse(await client.query(r'islist({});'))
        self.assertFalse(await client.query('islist(x[1]);'))

    async def test_isnan(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
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

    async def test_israw(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
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

    async def test_istuple(self, client):
        await client.query('x = [[0, 1], nil]; y = x[0];')
        with self.assertRaisesRegex(
                BadRequestError,
                'function `istuple` takes 1 argument but 0 were given'):
            await client.query('istuple();')

        self.assertFalse(await client.query('istuple([]);'))
        self.assertFalse(await client.query('istuple(x);'))
        self.assertFalse(await client.query('istuple(y);'))
        self.assertTrue(await client.query('istuple(x[0]);'))
        self.assertFalse(await client.query('istuple(0);'))
        self.assertFalse(await client.query('istuple("test");'))
        self.assertFalse(await client.query(r'istuple({});'))
        self.assertFalse(await client.query('istuple(x[1]);'))

    async def test_isutf8(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `isutf8` takes 1 argument but 0 were given'):
            await client.query('isutf8();')

        self.assertTrue(await client.query('isutf8( "pi" ); '))
        self.assertTrue(await client.query('isutf8( "" ); '))
        self.assertTrue(await client.query('isutf8( "ԉ" ); '))
        self.assertTrue(await client.query('isstr( "ԉ" ); '))  # alias isstr
        self.assertFalse(await client.query('isutf8([]);'))
        self.assertFalse(await client.query('isutf8(nil);'))
        self.assertFalse(await client.query('isstr(123);'))  # alias isstr
        self.assertFalse(await client.query(
                'isutf8(blob(0));',
                blobs=(pickle.dumps('binary'), )))

    async def test_len(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `len`'):
            await client.query('nil.len();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `len` takes 0 arguments but 1 was given'):
            await client.query('len(nil);')

        self.assertEqual(await client.query(r'{}.len();'), 0)
        self.assertEqual(await client.query(r'"".len();'), 0)
        self.assertEqual(await client.query(r'[].len();'), 0)
        self.assertEqual(await client.query(r'[[]][0].len();'), 0)
        self.assertEqual(await client.query(r'{x:0, y:1}.len();'), 2)
        self.assertEqual(await client.query(r'"xy".len();'), 2)
        self.assertEqual(await client.query(r'["x", "y"].len();'), 2)
        self.assertEqual(await client.query(r'[["x", "y"]][0].len();'), 2)

    async def test_lower(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `lower`'):
            await client.query('nil.lower();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `lower` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".lower(nil);')

        self.assertEqual(await client.query('"".lower();'), "")
        self.assertEqual(await client.query('"l".lower();'), "l")
        self.assertEqual(await client.query('"HI !!".lower();'), "hi !!")

    async def test_map(self, client):
        await client.query(r'''
            iris = {
                name: 'Iris',
                age: 6,
                likes: ['k3', 'swimming', 'red', 6],
            };
        ''')
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `map`'):
            await client.query('nil.map(||nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `map` takes 1 argument but 0 were given'):
            await client.query('map();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `map` expects argument 1 to be a `closure` '
                'but got type `nil` instead'):
            await client.query('map(nil);')

        self.assertEqual(await client.query(r'[].map(||nil)'), [])
        self.assertEqual(await client.query(r'{}.map(||nil)'), [])

        self.assertEqual(
            set(await client.query('iris.map(|k|k.upper());')),
            set({'NAME', 'AGE', 'LIKES'}))

        self.assertEqual(
            set(await client.query('iris.map(|_, v| (isarray(v)) ? true:v);')),
            set({'Iris', 6, True}))

        self.assertEqual(
            await client.query('iris.likes.map(|v, i|[v, i])'),
            [['k3', 0], ['swimming', 1], ['red', 2], [6, 3]])

    async def test_now(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `now` takes 0 arguments but 2 were given'):
            await client.query('now(0, 1);')

        now = await client.query('now();')
        diff = time.time() - now
        self.assertGreater(diff, 0.0)
        self.assertLess(diff, 0.01)

    async def test_pop(self, client):
        await client.query('list = [1, 2, 3];')
        self.assertEqual(await client.query('list.pop()'), 3)
        self.assertEqual(await client.query('list.pop()'), 2)
        self.assertEqual(await client.query('list;'), [1])
        self.assertIs(await client.query('[nil].pop()'), None)

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `pop`'):
            await client.query('nil.pop();')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `pop`'):
            await client.query('a = [list]; a[0].pop();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `pop` takes 0 arguments but 1 was given'):
            await client.query('list.pop(nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'cannot use function `pop` while the list is in use'):
            await client.query('list.map(||list.pop());')

        with self.assertRaisesRegex(
                IndexError,
                'pop from empty array'):
            await client.query('[].pop();')

    async def test_push(self, client):
        await client.query('list = [];')
        self.assertEqual(await client.query('list.push("a")'), 1)
        self.assertEqual(await client.query('list.push("b", "c")'), 3)
        self.assertEqual(await client.query('list;'), ['a', 'b', 'c'])

        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `push`'):
            await client.query('nil.push();')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `push`'):
            await client.query('a = [list]; a[0].push(nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `push` requires at least 1 argument '
                'but 0 were given'):
            await client.query('list.push();')

        with self.assertRaisesRegex(
                BadRequestError,
                'cannot use function `push` while the list is in use'):
            await client.query('list.map(||list.push(4));')

    async def test_refs(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `raw` has no function `refs`'):
            await client.query('"".refs();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `refs` takes 1 argument but 0 were given'):
            await client.query('refs();')

        self.assertGreater(await client.query('refs(nil);'), 1)
        self.assertGreater(await client.query('refs(true);'), 1)
        self.assertGreater(await client.query('refs(false);'), 1)
        self.assertEqual(await client.query('refs(t(id()));'), 3)
        self.assertEqual(await client.query('refs("Test RefCount")'), 2)
        self.assertEqual(await client.query('x = "Test RefCount"; refs(x)'), 3)

    async def test_remove(self, client):
        await client.query('list = [1, 2, 3];')
        self.assertEqual(await client.query('list.remove(|x|(x>1));'), 2)
        self.assertEqual(await client.query('list.remove(|x|(x>1));'), 3)
        self.assertEqual(await client.query('list;'), [1])
        self.assertIs(await client.query('list.remove(||false);'), None)
        self.assertEqual(await client.query('list.remove(||false, "");'), '')
        self.assertIs(await client.query('[].remove(||true);'), None)
        self.assertEqual(await client.query('["pi"].remove(||true);'), 'pi')

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
                'function `remove` takes at most 2 arguments '
                'but 3 were given'):
            await client.query('list.remove(||true, 2, 3);')

        with self.assertRaisesRegex(
                BadRequestError,
                'cannot use function `remove` while the list is in use'):
            await client.query('list.map(||list.remove(||true));')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `remove` expects argument 1 to be a `closure` '
                'but got type `nil` instead'):
            await client.query('list.remove(nil);')

    async def test_rename(self, client):
        await client.query('x = 42;')

        with self.assertRaisesRegex(
                IndexError,
                'type `int` has no function `rename`'):
            await client.query('x.rename("a", "b");')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `rename` takes 2 arguments '
                'but 1 was given'):
            await client.query('rename("x")')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `rename` takes 2 arguments '
                'but 0 were given'):
            await client.query('rename()')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `rename` takes 2 arguments '
                'but 3 were given'):
            await client.query('rename("x", "y", "z")')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `rename` can only be used on things with an id > 0; '
                r'\(things which are assigned automatically receive an id\)'):
            await client.query('{x:1}.rename("x", "y");')

        with self.assertRaisesRegex(
                BadRequestError,
                r'cannot use `rename` while thing `#\d+` is in use'):
            await client.query('map( ||rename("x", "y") );')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `rename` expects argument 1 to be of '
                r'type `raw` but got type `nil` instead'):
            await client.query('rename(nil, "y")')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `rename` expects argument 1 to be a valid name.*'):
            await client.query('rename("", "y");')

        with self.assertRaisesRegex(
                IndexError,
                r'thing `#\d+` has no property `a`'):
            await client.query('rename("a", "b");')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `rename` expects argument 2 to be of '
                r'type `raw` but got type `nil` instead'):
            await client.query('rename("x", nil)')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `rename` expects argument 2 to be a valid name.*'):
            await client.query('rename("x", "!");')

        self.assertEqual(await client.query('rename("x", "y"); y;'), 42)

        with self.assertRaisesRegex(
                IndexError,
                r'`x` is undefined'):
            await client.query('x;')

        self.assertEqual(await client.query('rename("y", "y"); y;'), 42)
        self.assertEqual(await client.query('x = 6; rename("y", "x"); x;'), 42)

    async def test_ret(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `ret` takes 0 arguments but 1 was given'):
            await client.query('ret(nil);')

        self.assertIs(await client.query('ret();'), None)
        self.assertIs(await client.query('map(||true).ret();'), None)
        self.assertIs(await client.query('(x = 1).ret();'), None)
        self.assertEqual(await client.query('ret(); x;'), 1)

    async def test_splice(self, client):
        await client.query('list = [];')
        self.assertEqual(await client.query('list.splice(0, 0, "a")'), [])
        self.assertEqual(await client.query('list.splice(1, 0, "d", "e")'), [])
        self.assertEqual(await client.query('list.splice(1, 0, "b", "d")'), [])
        self.assertEqual(await client.query('list.splice(2, 1, "c")'), ['d'])
        self.assertEqual(await client.query('list.splice(0, 2)'), ['a', 'b'])
        self.assertEqual(await client.query('list;'), ['c', 'd', 'e'])

        self.assertEqual(await client.query('list.splice(-2, 1)'), ['d'])
        self.assertEqual(await client.query('list.splice(5, 1)'), [])
        self.assertEqual(await client.query('list.splice(-5, 1)'), [])
        self.assertEqual(await client.query('list.splice(0, -1)'), [])

        self.assertEqual(await client.query('list;'), ['c', 'e'])
        self.assertEqual(await client.query('list.splice(0, 10)'), ['c', 'e'])
        self.assertEqual(await client.query('list.splice(5, 10, 1, 2, 3)'), [])
        self.assertEqual(await client.query('list;'), [1, 2, 3])

        with self.assertRaisesRegex(
                IndexError,
                'type `thing` has no function `splice`'):
            await client.query('splice();')

        with self.assertRaisesRegex(
                IndexError,
                'type `tuple` has no function `splice`'):
            await client.query('a = [list]; a[0].splice(nil);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `splice` requires at least 2 arguments '
                'but 0 were given'):
            await client.query('list.splice();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `splice` requires at least 2 arguments '
                'but 1 was given'):
            await client.query('list.splice(0);')

        with self.assertRaisesRegex(
                BadRequestError,
                'cannot use function `splice` while the list is in use'):
            await client.query('list.map(||list.splice(0, 1));')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `splice` expects argument 1 to be of '
                r'type `int` but got type `float` instead'):
            await client.query('list.splice(0.0, 0)')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `splice` expects argument 2 to be of '
                r'type `int` but got type `nil` instead'):
            await client.query('list.splice(0, nil)')

    async def test_startswith(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `startswith`'):
            await client.query('nil.startswith("world!");')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `startswith` takes 1 argument '
                'but 2 were given'):
            await client.query('"Hi World!".startswith("x", 2)')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `startswith` takes 1 argument '
                'but 0 were given'):
            await client.query('"Hi World!".startswith()')

        with self.assertRaisesRegex(
                BadRequestError,
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
                BadRequestError,
                'function `str` takes 1 argument but 0 were given'):
            await client.query('str();')

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

    async def test_test(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `regex` has no function `test`'):
            await client.query('(/.*/).test();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `test` takes 1 argument but 0 were given'):
            await client.query('"".test();')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `test` expects argument 1 to be of '
                r'type `regex` but got type `raw` instead'):
            await client.query('"".test("abc");')

        self.assertTrue(await client.query(r'"".test(//);'))
        self.assertTrue(await client.query(r'"Hi".test(/hi/i);'))
        self.assertTrue(await client.query(r'"hello!".test(/hello.*/);'))
        self.assertFalse(await client.query(r'"Hi".test(/hi/);'))
        self.assertFalse(await client.query(r'"hello".test(/hello!.*/);'))

    async def test_t(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `t` requires at least 1 argument '
                'but 0 were given'):
            await client.query('t();')

        with self.assertRaisesRegex(
                BadRequestError,
                r'function `t` expects argument 1 to be of '
                r'type `int` but got type `nil` instead'):
            await client.query('t(nil);')

        with self.assertRaisesRegex(
                IndexError,
                'collection `stuff` has no `thing` with id -1'):
            await client.query('t(-1);')

        id = await client.query(r'(t = {x:42}).id();')
        t = await client.query('t({});'.format(id))
        self.assertEqual(t['x'], 42)
        self.assertFalse(await client.query(r'isarray(t(t.id()));'))
        self.assertTrue(await client.query(r'isarray(t(t.id(), ));'))
        stuff, t = await client.query('t(id(), {});'.format(id), deep=3)
        self.assertEqual(stuff['t'], t)

    async def test_try(self, client):
        with self.assertRaisesRegex(
                BadRequestError,
                'function `try` requires at least 1 argument '
                'but 0 were given'):
            await client.query('try();')

        self.assertEqual(await client.query('try( (10 // 2) );'), 5)
        self.assertEqual(await client.query('try( (10 // 0) );'), None)
        self.assertIs(await client.query('try( (10 // 0), false );'), False)
        self.assertEqual(
            await client.query('try((10 // 0), nil, "ZERO_DIV_ERROR");'), None)
        self.assertEqual(await client.query('try((10 // 0), nil, -97);'), None)
        self.assertEqual(await client.query('try((10 // 0), nil, 97);'), None)
        with self.assertRaisesRegex(
                IndexError,
                'unknown error: `UNKNOWN`, see https:.*'):
            await client.query('try( (10 // 0), nil, "UNKNOWN");')

        with self.assertRaisesRegex(
                IndexError,
                'unknown error number: 0, see https:.*'):
            await client.query('try( (10 // 0), nil, 0);')

        with self.assertRaisesRegex(
                BadRequestError,
                'cannot convert type `nil` to an `errnr`'):
            await client.query('try( (10 // 0), nil, nil);')

        with self.assertRaisesRegex(
                ZeroDivisionError,
                'division or modulo by zero'):
            await client.query('try( (10 // 0), nil, "INDEX_ERROR");')

    async def test_upper(self, client):
        with self.assertRaisesRegex(
                IndexError,
                'type `nil` has no function `upper`'):
            await client.query('nil.upper();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `upper` takes 0 arguments but 1 was given'):
            await client.query('"Hello World".upper(nil);')

        self.assertEqual(await client.query('"".upper();'), "")
        self.assertEqual(await client.query('"U".upper();'), "U")
        self.assertEqual(await client.query('"hi !!".upper();'), "HI !!")


if __name__ == '__main__':
    run_test(TestCollectionFunctions())
