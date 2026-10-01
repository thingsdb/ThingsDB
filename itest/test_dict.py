#!/usr/bin/env python
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import ValueError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError
from lib.vars import THINGSDB_MEMCHECK


class TestDict(TestBase):

    title = 'Test dict type'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):

        self.node0.version()

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_dict_init(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `dict` takes at most 1 argument but 2 were given'):
            await q('dict([], nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `dict` expects argument 1 to be of '
                'type `list` or `tuple` but got type `nil` instead'):
            await q('dict(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot use type `nil` as a dictionary key'):
            await q('dict([[nil, nil]]);')

        with self.assertRaisesRegex(
                ValueError,
                r'type `dict` must be initialized with a list of '
                r'\[key, value\] pairs'):
            await q('dict([["a", nil, nil]]);')

        await q(r"""//ti
            .d = dict([
                [uuid('00000000-0000-0000-0000-000000000000'), "uuid"],
                [42, "int"],
                ["foo", "str"],
            ]);
        """)
        res = await q('.d;')
        self.assertEqual(len(res), 3)
        self.assertIn(['00000000-0000-0000-0000-000000000000', "uuid"], res)
        self.assertIn([42, "int"], res)
        self.assertIn(["foo", "str"], res)

    async def test_dict_int_keys(self, client):
        q = client.query
        await q(r"""//ti
            set_type('Root', {
                d: 'dict<int:any>'
            });
            .to_type('Root');
            range(-11111, 11111, 11).each(|i| .d[i] = `key {i}`);
        """)
        res = await q('.d;')
        self.assertEqual(sorted(res),
                         [[i, f'key {i}'] for i in range(-11111, 11111, 11)])

        self.assertEqual((await q('.d[-11111];')), 'key -11111')
        res = await q(r"""//ti
            s = "overwrite";
            .d[-11111] = s;
            .d[-11111] = s;  // dubble overwrite to test ref count
        """)
        self.assertEqual((await q('.d[-11111];')), 'overwrite')

        with self.assertRaisesRegex(
                LookupError,
                r'key `1` not found'):
            await q('.d[1]')

        with self.assertRaisesRegex(
                TypeError,
                r'dict key must be of type `int`, got `str`'):
            await q('.d["1"] = "new";')

    async def test_dict_uuid_keys(self, client):
        q = client.query
        uuids = await q('range(1000).map(|| uuid());')

        await q(r"""//ti
            set_type('Root', {
                d: 'dict<uuid:any>'
            });
            .to_type('Root');
            uuids.map(|u| uuid(u)).each(|u| .d[u] = `key {u}`);
        """, uuids=uuids)
        res = await q('.d;')
        self.assertEqual(sorted(res),
                         [[i, f'key {i}'] for i in sorted(uuids)])

        with self.assertRaisesRegex(
                LookupError,
                r'key `00000000-0000-0000-0000-000000000000` not found'):
            await q('.d[uuid(uuid)]',
                    uuid='00000000-0000-0000-0000-000000000000')

        u = uuids[0]
        self.assertEqual((await q('.d[uuid(u)];', u=u)), f'key {u}')
        res = await q(r"""//ti
            s = "overwrite";
            .d[uuid(u)] = s;
            .d[uuid(u)] = s;  // dubble overwrite to test ref count
        """, u=u)
        self.assertEqual((await q('.d[uuid(u)];', u=u)), 'overwrite')

        with self.assertRaisesRegex(
                TypeError,
                r'dict key must be of type `uuid`, got `int`'):
            await q('.d[1] = "new";')

    async def test_dict_str_keys(self, client):
        q = client.query
        long = 'x'*9999
        await q(r"""//ti
            set_type('Root', {
                d: 'dict<str:any>'
            });
            .to_type('Root');
            .d[""] = "empty";
            .d["#"] = "hash";
            .d[long] = long;
            nil;
        """, long=long)

        res = await q('.d;')
        self.assertEqual(len(res), 3)
        self.assertIn(["", "empty"], res)
        self.assertIn(["#", "hash"], res)
        self.assertIn([long, long], res)

        with self.assertRaisesRegex(
                LookupError,
                r'key `a` not found'):
            await q('.d["a"]')

        with self.assertRaisesRegex(
                TypeError,
                r'dict key must be of type `str`, got `int`'):
            await q('.d[1] = "new";')

        self.assertEqual(await q('.d[""];'), "empty")
        self.assertEqual(await q('.d["#"];'), "hash")

        res = await q(r"""//ti
            s = "overwrite";
            .d[''] = s;
            .d[""] = s;  // dubble overwrite to test ref count
        """)
        self.assertEqual((await q('.d[""];')), 'overwrite')

    async def test_dict_clear(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `clear` takes 0 arguments but 1 was given'):
            await q('dict().clear(nil);')

        await q('.d = dict([[1, 1], [2, 2]]);')
        res = await q('.d.clear();')
        self.assertIs(res, None)
        res = await q('.d.len();')
        self.assertEqual(res, 0)

    async def test_dict_copy(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `copy` takes at most 1 argument but 2 were given'):
            await q('dict().copy(1, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting `deep` to be of type `int` but '
                r'got type `nil` instead'):
            await q('dict().copy(nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a `deep` value between 0 '
                r'and 127 but got -1 instead'):
            await q('dict().copy(-1);')

        res = await q(r"""//ti
            new_type('T');
            .d = dict([['a', {n: {}, t: T{}}], ['b', {}]]);
            .dd = .d;  // This should be a copy too
            .d0 = .d.copy(0);  // Copy deep=0
            .dn = .d.copy();   // Copy deep=0 (implicit)
            .d1 = .d.copy(1);  // Copy deep=1
            .d2 = .d.copy(3);  // Copy deep=2
            d = .d;  // This is by ref
            dd = .d.copy();  // This is again a copy
            dd['d'] = .d;  // This is not a copy, but to tuple
            [
              is_dict(.d),              // true
              is_dict(.dd),             // true
              is_dict(d),               // true
              is_dict(dd),              // true
              is_dict(dd['d']),         // false
              is_tuple(dd['d']),        // true
              .dd == .d,                // false
              d == .d,                  // true
              dd == .d,                 // false
              .d0 == .d,                // false
              .d1 == .d,                // false
              .d['a'] == .dd['a'],      // true
              .d['a'] == .d0['a'],      // true
              .d['a'] == .dn['a'],      // true
              .d['a'] == .d1['a'],      // false
              .d['a'].n == .d0['a'].n,  // true
              .d['a'].n == .d1['a'].n,  // true
              .d['a'].n == .d2['a'].n,  // false
              type(.d1['a'].t),         // 'T'
              type(.d2['a'].t),         // 'thing'
            ];
        """)
        self.assertEqual(res, [
            True,
            True,
            True,
            True,
            False,
            True,
            False,
            True,
            False,
            False,
            False,
            True,
            True,
            True,
            False,
            True,
            True,
            False,
            'T',
            'thing'])

    async def test_dict_del(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `del` takes 1 argument but 0 were given'):
            await q('dict().del();')

        with self.assertRaisesRegex(
                LookupError,
                r'key of type `float` not found'):
            await q('dict().del(1.0);')

        with self.assertRaisesRegex(
                LookupError,
                r'key `123` not found'):
            await q('dict().del(123);')

        self.assertEqual(await q('dict([[0, 42]]).del(0);'), 42)
        res = await q('d = dict([[0, 42], [1, 43]]); d.del(0); d.len()')
        self.assertEqual(res, 1)

    async def test_dict_dup(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `dup` takes at most 1 argument but 2 were given'):
            await q('dict().dup(1, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting `deep` to be of type `int` but '
                r'got type `nil` instead'):
            await q('dict().dup(nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a `deep` value between 0 '
                r'and 127 but got -1 instead'):
            await q('dict().dup(-1);')

        res = await q(r"""//ti
            new_type('T');
            .d = dict([['a', {n: {}, t: T{}}], ['b', {}]]);
            .dd = .d;  // This should be a copy too
            .d0 = .d.dup(0);  // Duplicate deep=0
            .dn = .d.dup();   // Duplicate deep=0 (implicit)
            .d1 = .d.dup(1);  // Duplicate deep=1
            .d2 = .d.dup(2);  // Duplicate deep=2
            d = .d;  // This is by ref
            dd = .d.dup();  // This is again a duplicate
            dd['d'] = .d;  // This is not a duplicate, but to tuple
            [
              is_dict(.d),              // true
              is_dict(.dd),             // true
              is_dict(d),               // true
              is_dict(dd),              // true
              is_dict(dd['d']),         // false
              is_tuple(dd['d']),        // true
              .dd == .d,                // false
              d == .d,                  // true
              dd == .d,                 // false
              .d0 == .d,                // false
              .d1 == .d,                // false
              .d['a'] == .dd['a'],      // true
              .d['a'] == .d0['a'],      // true
              .d['a'] == .dn['a'],      // true
              .d['a'] == .d1['a'],      // false
              .d['a'].n == .d0['a'].n,  // true
              .d['a'].n == .d1['a'].n,  // true
              .d['a'].n == .d2['a'].n,  // false
              type(.d1['a'].t),         // 'T'
              type(.d2['a'].t),         // 'T'
            ];
        """)
        self.assertEqual(res, [
            True,
            True,
            True,
            True,
            False,
            True,
            False,
            True,
            False,
            False,
            False,
            True,
            True,
            True,
            False,
            True,
            True,
            False,
            'T',
            'T'])

    async def test_dict_each(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `each` takes 1 argument but 0 were given'):
            await q('dict().each();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `each` expects argument 1 to be of '
                r'type `closure` but got type `nil` instead'):
            await q('dict().each(nil);')

        r0, r1 = await q(r"""//ti
            d = dict(range(-99999, 0).map(|x| [x, x]));
            [
                timeit(d.each(|k, v| v)),
                timeit(d.each(|k, v| k)),
            ];
        """)

        if THINGSDB_MEMCHECK:
            # looping over keys takes more time; only messure when memcheck
            # is on, otherwise both are too fast to mesure fair
            self.assertLess(r0['time'], r1['time'])

        self.assertIs(r0['data'], None)
        self.assertIs(r1['data'], None)

    async def test_for_in(self, client):
        q = client.query

        r0, r1 = await q(r"""//ti
            d = dict(range(-99999, 0).map(|x| [x, x]));
            [
                timeit({r = 0; for (k, v in d) r=v<r?v:r; r;}),
                timeit({r = 0; for (k, v in d) r=k<r?k:r; r;}),
            ];
        """)

        if THINGSDB_MEMCHECK:
            # looping over keys takes more time; only messure when memcheck
            # is on, otherwise both are too fast to mesure fair
            self.assertLess(r0['time'], r1['time'])

        self.assertEqual(r0['data'], -99999)
        self.assertEqual(r1['data'], -99999)

    async def test_dict_get(self, client):
        q = client.query
        u = '00000000-0000-0000-0000-000000000000'
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `get` requires at least 1 argument '
                'but 0 were given'):
            await q('dict().get();')

        self.assertIs(await q('dict().get(1, nil);'), None)
        self.assertIs(await q('dict().get(nil, false);'), False)
        self.assertIs(await q('dict([[-42, true]]).get(-42, false);'), True)
        self.assertIs(await q('dict([[-42, true]]).get(42);'), None)
        self.assertIs(await q('dict([["a", true]]).get(42);'), None)
        self.assertIs(await q('dict([["a", true]]).get("a");'), True)
        self.assertIs(await q('u=uuid(u);dict([[u,true]]).get(u)', u=u), True)

    async def test_dict_index(self, client):
        q = client.query
        u = '00000000-0000-0000-0000-000000000000'
        with self.assertRaisesRegex(
                TypeError,
                'type `dict` has no slice support'):
            await q('dict()[0:10]')

        self.assertIs(await q('try(dict()[1])||nil'), None)
        self.assertIs(await q('try(dict()[nil])||false'), False)
        res, n = await q(r"""//ti
            d = dict();
            u = uuid(u);
            d[u] = true;
            d['t'] = 'Hello';
            assert({d[42] = 6} == 6);  // test return values
            d[u] = !d[u];
            d['t'] += ' World!';
            assert({d[42] *= 7} == 42);  // test return values
            [d, d.len()];
        """, u=u)
        self.assertEqual(n, len(res))
        self.assertEqual(res, [
            ['00000000-0000-0000-0000-000000000000', False],
            [42, 42],
            ['t', 'Hello World!'],
        ])

    async def test_dict_has(self, client):
        q = client.query
        u = '00000000-0000-0000-0000-000000000000'
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `has` takes 1 argument but 0 were given'):
            await q('dict().has();')

        self.assertTrue(await q('dict([[0, 0]]).has(0)'))
        self.assertTrue(await q('u=uuid(u);dict([[u, 0]]).has(u)', u=u))
        self.assertTrue(await q('dict([["", 0]]).has("")'))
        self.assertFalse(await q('dict([[1, 0]]).has(0)'))
        self.assertFalse(await q('u=uuid(u);dict([[uuid(), 0]]).has(u)', u=u))
        self.assertFalse(await q('dict([[" ", 0]]).has("")'))

    async def test_dict_len(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `len` takes 0 arguments but 1 was given'):
            await q('dict().len(nil);')
        # len() is tested throughout other tests

    async def test_dict_map(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `map` takes 1 argument but 0 were given'):
            await q('dict().map();')

        with self.assertRaisesRegex(
                TypeError,
                'function `map` expects argument 1 to be of '
                'type `closure` but got type `int` instead'):
            await q('dict().map(0);')

        self.assertEqual(await q('dict().map(||nil)'), [])

        n = 10
        keys, values = await q("""//ti
            d = dict(range(n).map(|i| [`key: {i}`, `value: {i}`]));
            [
                d.map(|k| k).sort(),
                d.map(|_, v| v).sort(),
            ];
        """, n=n)
        self.assertEqual(keys, [f'key: {i}' for i in range(n)])
        self.assertEqual(values, [f'value: {i}' for i in range(n)])

    async def test_dict_restriction(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `restriction` takes 0 arguments but 1 was given'):
            await q('dict().restriction(nil);')

        res = await q(r"""//ti
            set_type('Root', {
                d: 'dict',
                k: 'dict<int:any>',
                v: 'dict<any:int>',
                b: 'dict<int:int>',
                c: 'dict<uuid:uint>',
            });
            .to_type('Root');
            [
                .d.restriction(),
                .k.restriction(),
                .v.restriction(),
                .b.restriction(),
                .c.restriction(),
            ]
        """)
        self.assertEqual(res, [
            ['any', 'any'],
            ['int', 'any'],
            ['any', 'int'],
            ['int', 'int'],
            ['uuid', 'uint'],
        ])
        with self.assertRaisesRegex(
                ValueError,
                'dict requires values to be integers greater '
                'than or equal to 0'):
            await q('.c.set(uuid(), -1)')

    async def test_dict_set(self, client):
        q = client.query
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `set` takes 2 arguments but 1 was given'):
            await q('dict().set(nil);')

        self.assertEqual(await q('dict().set(0, 42)'), 42)
        self.assertEqual(await q('d=dict(); d.set(0, 42); d;'), [[0, 42]])

    async def test_wrap_as_arr(self, client):
        q = client.query
        res = await q("""//ti
            set_type('Person', {name: 'str'});
            set_type('People', {people: '&[Person]'});
            t = { people: dict([["iris", Person{name: 'Iris'}]]) };
            t.wrap('People');
        """)
        self.assertEqual(res, {'people': [{'name': 'Iris'}]})

        await q("""//ti
            set_type('R', {
                users: 'dict<uuid:str>'
            });
            set_type('_R', {
                users: '[str]'
            }, WPO);
            .to_type('R');
            .users[uuid()] = 'Iris';
            .users[uuid()] = 'Sasha';
        """)
        users = sorted((await q('.wrap("_R")'))['users'])
        self.assertEqual(users, ['Iris', 'Sasha'])

    async def test_wrap_as_dict(self, client):
        q = client.query

        res = await q("""//ti
            set_type('Person', {name: 'str'});
            set_type('People', {people: '&[Person]'});
            t = { people: dict([["iris", Person{name: 'Iris'}]]) };
            t.wrap('People');
        """)
        self.assertEqual(res, {'people': [{'name': 'Iris'}]})

        await q("""//ti
            set_type('P', {name: 'str'}, HID);
            set_type('R', {
                users: 'dict<int:str>',
                people: 'dict<int:P>',
            });
            set_type('_A', {
                users: 'dict<int:str>'
            }, WPO|HID);
            set_type('_B', {
                users: 'dict<str:str>'
            }, WPO|HID);
            set_type('_C', {
                users: 'dict<int:int>'
            }, WPO|HID);
            set_type('_W', {
                people: '&[P]'
            }, WPO|HID);
            .to_type('R');
            .users[0] = 'Iris';
            .users[1] = 'Sasha';
            for (id, name in .users) {
                .people.set(id, P{name:,});
            };
        """)
        res = await q('.wrap("_A")')
        self.assertIn('users', res)
        self.assertEqual(res['users'], [[0, 'Iris'], [1, 'Sasha']])
        res = await q('.wrap("_B")')
        self.assertNotIn('users', res)
        res = await q('.wrap("_C")')
        self.assertNotIn('users', res)
        res = await q("""//ti
            .wrap(&{
                people: [{
                    name: 'str'
                }]
            });
        """)
        self.assertEqual(res, {
            'people': [{'name': 'Iris'}, {'name': 'Sasha'}]
        })
        res = await q('.wrap("_W")')
        self.assertEqual(res, {
            'people': [{'name': 'Iris'}, {'name': 'Sasha'}]
        })


if __name__ == '__main__':
    run_test(TestDict())
