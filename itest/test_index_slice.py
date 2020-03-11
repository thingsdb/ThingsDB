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
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError


class TestIndexSlice(TestBase):

    title = 'Test index and slices'

    @default_test_setup(num_nodes=3, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.set_default_scope('//stuff')

        # uncomment for quick test
        # return (await self.run_tests(client0, client0, client0))

        # add another node for query validation
        await self.node1.join_until_ready(client0)
        await self.node2.join_until_ready(client0)

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')

        client2 = await get_client(self.node2)
        client2.set_default_scope('//stuff')

        await self.run_tests(client0, client1, client2)

        # expected no garbage collection
        for client in (client0, client1, client2):
            counters = await client.query('counters();', scope='@node')
            self.assertEqual(counters['garbage_collected'], 0)
            self.assertEqual(counters['events_failed'], 0)

            client.close()
            await client.wait_closed()

    async def test_example_doc(self, client0, client1, client2):
        res = await client0.query(r'''
            months = ['January', 'February', 'XXX', 'May'];

            /* Replace 'XXX' with 'March' and 'April' */
            months[2:3] = ['March', 'April'];

            /* Return result */
            months;
        ''')

        self.assertEqual(res, ['January', 'February', 'March', 'April', 'May'])

    async def test_illigal(self, client0, client1, client2):
        s = 'abcdef'
        tu = [1, 2, 3, 4, 5, 6]
        await client0.query(f'.raw = "{s}";')
        await client0.query(f'.list = [{tu}];')

        await asyncio.sleep(0.2)

        with self.assertRaisesRegex(
                TypeError,
                r'type `str` does not support index assignments'):
            await client0.query('.raw[0] = "z";')

        with self.assertRaisesRegex(
                TypeError,
                r'type `tuple` does not support index assignments'):
            await client0.query('.list[0][0] = "z";')

    async def test_thing_get(self, client0, client1, client2):
        await client0.query(r'''
            .ti = {
                name: 'Iris',
                age: 6,
                likes: ['swimming'],
            };
        ''')
        await asyncio.sleep(0.2)

        with self.assertRaisesRegex(
                LookupError,
                r'thing `#\d+` has no property `x`'):
            await client0.query('.ti["x"];')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting an index of type `str` '
                r'but got type `int` instead'):
            await client0.query(f'.ti[4];')

        self.assertEqual(await client0.query('.ti["name"];'), 'Iris')
        self.assertEqual(await client0.query('.ti["age"];'), 6)
        self.assertEqual(await client0.query('.ti["likes"];'), ['swimming'])

        await client0.query('.ti["likes"][0] = "Cato";')
        await asyncio.sleep(0.2)

        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.ti["likes"];'), ['Cato'])

    async def test_thing_set(self, client0, client1, client2):
        await client0.query(r'''
            .ti = {
                name: 'Iris',
                age: 6,
                likes: ['swimming'],
            };
        ''')
        await asyncio.sleep(0.2)

        await client0.query('.ti["name"] = "Cato";')
        await asyncio.sleep(0.2)

        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.ti.name;'), 'Cato')

        await client0.query('.ti["age"] -= 1;')
        await asyncio.sleep(0.2)

        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.ti.age;'), 5)

        await client0.query('.ti["hairColor"] = "blonde";')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.ti.hairColor;'), 'blonde')

    async def test_from_raw_by_index(self, client0, client1, client2):
        s = 'abcdef'
        n = len(s)
        await client0.query(f'.raw = "{s}";')

        await asyncio.sleep(0.2)
        for i in range(-n, n):
            self.assertEqual(await client0.query(f'.raw[{i}]'), s[i])

        with self.assertRaisesRegex(
                LookupError,
                'index out of range'):
            await client0.query(f'.raw[{n}];')

        with self.assertRaisesRegex(
                LookupError,
                'index out of range'):
            await client0.query(f'.raw[{-(n+1)}];')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting an index of type `int` '
                r'but got type `nil` instead'):
            await client0.query(f'.raw[nil];')

    async def test_from_raw_by_slice(self, client0, client1, client2):
        s = 'abcdef'
        n = len(s)
        await client0.query(f'.raw = "{s}";')

        await asyncio.sleep(0.2)
        self.assertTrue(await client0.query('(.raw[] == .raw);'))
        self.assertTrue(await client0.query('(.raw[:] == .raw);'))
        self.assertTrue(await client0.query('(.raw[::] == .raw);'))
        self.assertTrue(await client0.query('(.raw[::] == .raw);'))
        self.assertTrue(await client0.query('(.raw[nil:nil:nil] == .raw);'))
        self.assertTrue(await client0.query('(.raw[0::] == .raw);'))
        self.assertTrue(await client0.query('(.raw[-100:100:1] == .raw);'))

        self.assertEqual(await client0.query('.raw[10:]'), s[10:])
        self.assertEqual(await client0.query('.raw[1:3]'), s[1:3])
        self.assertEqual(await client0.query('.raw[-3:]'), s[-3:])
        self.assertEqual(await client0.query('.raw[:-3]'), s[:-3])
        self.assertEqual(await client0.query('.raw[:4:]'), s[:4:])
        self.assertEqual(await client0.query('.raw[-3:-1]'), s[-3:-1])
        self.assertEqual(await client0.query('.raw[::-1]'), s[::-1])
        self.assertEqual(await client0.query('.raw[::2]'), s[::2])
        self.assertEqual(await client0.query('.raw[1::2]'), s[1::2])

    async def test_from_arr_by_index(self, client0, client1, client2):
        li = [1, 2, 3, 4, 5, 6]
        n = len(li)
        await client0.query(f'.list = {li};')

        await asyncio.sleep(0.2)
        for i in range(-n, n):
            self.assertEqual(await client0.query(f'.list[{i}]'), li[i])

        with self.assertRaisesRegex(
                ValueError,
                r'slice step cannot be zero'):
            await client0.query(f'.list[0:0:0];')

        with self.assertRaisesRegex(
                LookupError,
                'index out of range'):
            await client0.query(f'.list[{n}];')

        with self.assertRaisesRegex(
                LookupError,
                'index out of range'):
            await client0.query(f'.list[{-(n+1)}];')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting an index of type `int` '
                r'but got type `nil` instead'):
            await client0.query(f'.list[nil];')

    async def test_from_arr_by_slice(self, client0, client1, client2):
        li = [1, 2, 3, 4, 5, 6]
        n = len(li)
        await client0.query(f'.list = {li};')

        await asyncio.sleep(0.2)
        self.assertTrue(await client0.query('(.list[] == .list);'))
        self.assertTrue(await client0.query('(.list[:] == .list);'))
        self.assertTrue(await client0.query('(.list[::] == .list);'))
        self.assertTrue(await client0.query('(.list[::] == .list);'))
        self.assertTrue(await client0.query('(.list[nil:nil:nil] == .list);'))
        self.assertTrue(await client0.query('(.list[0::] == .list);'))
        self.assertTrue(await client0.query('(.list[-100:100:1] == .list);'))

        self.assertEqual(await client0.query('.list[10:]'), li[10:])
        self.assertEqual(await client0.query('.list[-3:]'), li[-3:])
        self.assertEqual(await client0.query('.list[1:3]'), li[1:3])
        self.assertEqual(await client0.query('.list[:-3]'), li[:-3])
        self.assertEqual(await client0.query('.list[:4:]'), li[:4:])
        self.assertEqual(await client0.query('.list[-3:-1]'), li[-3:-1])
        self.assertEqual(await client0.query('.list[::-1]'), li[::-1])
        self.assertEqual(await client0.query('.list[::2]'), li[::2])
        self.assertEqual(await client0.query('.list[1::2]'), li[1::2])

    async def test_set_arr_by_index(self, client0, client1, client2):
        li = [1, 2, 3, 4, 5, 6]
        to = ['a', 'b', 'c', 'd', 'e', 'f']
        n = len(li)
        await client0.query(f'.list = {li};')

        await asyncio.sleep(0.2)
        for i, c in enumerate(to):
            self.assertEqual(await client0.query(f'.list[{i}] = "{c}";'), c)

        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.list'), to)

        await client.query('.list[0] += "bc";')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.list[0]'), 'abc')

        await client0.query(r'.list[0] = [{}, {}];')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertTrue(await client.query('istuple(.list[0]);'))

    async def test_reference(self, client0, client1, client2):
        res = await client0.query(r'''
            a = range(2);
            b = range(2);
            c = range(2);
            .a = a;
            .set('b', b);
            thing(.id())['c'] = c;
            aa = a;
            bb = b;
            cc = c;
            a.push(3);
            b.push(3);
            c.push(3);
            .a.push(4);
            .get('b').push(4);
            thing(.id())['c'].push(4);
            aa.push(5);
            bb.push(5);
            cc.push(5);
            [a, b, c, .a, .b, .c, aa, bb, cc];
        ''')
        self.assertEqual(
            res, [
                [0, 1, 3, 5], [0, 1, 3, 5], [0, 1, 3, 5],
                [0, 1, 4], [0, 1, 4], [0, 1, 4],
                [0, 1, 3, 5], [0, 1, 3, 5], [0, 1, 3, 5]
            ]
        )

        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(
                await client0.query('[.a, .b, .c];'),
                [[0, 1, 4], [0, 1, 4], [0, 1, 4]])

    async def test_set_arr_by_slice(self, client0, client1, client2):
        li = [1, 2, 5, 6]
        n = len(li)
        await client0.query(f'.list = {li};')

        await asyncio.sleep(0.2)
        with self.assertRaisesRegex(
                ValueError,
                r'slice assignments require a step '
                r'value of 1 but got -1 instead'):
            await client0.query(f'.list[::-1] = [];')

        with self.assertRaisesRegex(
                TypeError,
                r'slice assignments require a `list` or `tuple` type '
                r'but got type `int` instead'):
            await client0.query(f'.list[] = 42;')

        with self.assertRaisesRegex(
                TypeError,
                r'unsupported operand type `\*\=` for slice assignments'):
            await client0.query(f'.list[] *= [];')

        await client0.query('.list[2:0] = [3, 4];')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.list'), [1, 2, 3, 4, 5, 6])

        await client0.query('.list[] = [1, 7, 4];')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.list'), [1, 7, 4])

        await client0.query('.list[1:2] = [2, 3];')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.list'), [1, 2, 3, 4])

        await client0.query('.list[10:] = [5, 6];')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.list'), [1, 2, 3, 4, 5, 6])

        await client0.query('.list[0:0] = [0];')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.list'), list(range(7)))

        await client0.query('.list[-3:] = [];')
        await asyncio.sleep(0.2)
        for client in (client0, client1, client2):
            self.assertEqual(await client.query('.list'), [0, 1, 2, 3])


if __name__ == '__main__':
    run_test(TestIndexSlice())
