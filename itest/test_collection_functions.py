#!/usr/bin/env python
import asyncio
import pickle
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.target import Target
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import BadRequestError
from thingsdb.exceptions import IndexError
from thingsdb.exceptions import OverflowError


class TestCollectionFunctions(TestBase):

    title = 'Test collection functions'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        await self.run_tests(client)

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

        self.assertEqual(await client.query('assert((2>1)); 42;'), 42)

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` requires at least 1 argument '
                'but 0 were given'):
            await client.query('assert();')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `assert` expects at most 3 arguments '
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
        self.assertEqual(await client.query(
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

        self.assertEqual(await client.query(r'del("greet");'), None)
        self.assertEqual(await client.query(r'hasprop("greet");'), False)

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
                'function `find` expects at most 2 arguments '
                'but 3 were given'):
            await client.query('x.find(||true, 2, 3);')

        with self.assertRaisesRegex(
                BadRequestError,
                'function `find` expects argument 1 to be a `closure` '
                'but got type `int` instead'):
            await client.query('x.find(0);')

        self.assertEqual(await client.query('[].find(||true);'), None)
        self.assertEqual(await client.query('[].find(||false);'), None)
        self.assertEqual(await client.query('x.find(||true);'), 42)
        self.assertEqual(await client.query('x.find(|v|(v==42));'), 42)
        self.assertEqual(await client.query('x.find(|_,i|(i>=0));'), 42)
        self.assertEqual(await client.query('x.find(|_,i|(i>=1));'), "gallaxy")
        self.assertEqual(await client.query('x.find(||nil);'), None)
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

        self.assertEqual(await client.query('[].findindex(||true);'), None)
        self.assertEqual(await client.query('[].findindex(||false);'), None)
        self.assertEqual(await client.query('x.findindex(||true);'), 0)
        self.assertEqual(await client.query('x.findindex(||false);'), None)
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
        self.assertEqual(await client.query('x.indexof(42.1);'), None)
        self.assertEqual(await client.query('x.indexof("ThingsDb");'), None)
        self.assertEqual(await client.query('x.indexof(true);'), None)
        self.assertEqual(await client.query(r'x.indexof({});'), None)

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

    async def test_remove(self, client):
        await client.query('list = [1, 2, 3];')
        self.assertEqual(await client.query('list.remove(|x|(x>1));'), 2)
        self.assertEqual(await client.query('list.remove(|x|(x>1));'), 3)
        self.assertEqual(await client.query('list;'), [1])
        self.assertEqual(await client.query('list.remove(||false);'), None)
        self.assertEqual(await client.query('list.remove(||false, "");'), '')
        self.assertEqual(await client.query('[].remove(||true);'), None)
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
                'function `remove` expects at most 2 arguments '
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


if __name__ == '__main__':
    run_test(TestCollectionFunctions())
