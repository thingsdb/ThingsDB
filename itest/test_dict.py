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
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `dict` takes at most 1 argument but 2 were given'):
            await client.query('dict([], nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `dict` expects argument 1 to be of '
                'type `list` or `tuple` but got type `nil` instead'):
            await client.query('dict(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot use type `nil` as a dictionary key'):
            await client.query('dict([[nil, nil]]);')

        with self.assertRaisesRegex(
                ValueError,
                r'type `dict` must be initialized with a list of '
                r'\[key, value\] pairs'):
            await client.query('dict([["a", nil, nil]]);')

        await client.query(r"""//ti
            .d = dict([
                [uuid('00000000-0000-0000-0000-000000000000'), "uuid"],
                [42, "int"],
                ["foo", "str"],
            ]);
        """)
        res = await client.query('.d;')
        self.assertEqual(len(res), 3)
        self.assertIn(['00000000-0000-0000-0000-000000000000', "uuid"], res)
        self.assertIn([42, "int"], res)
        self.assertIn(["foo", "str"], res)

    async def test_dict_int_keys(self, client):
        await client.query(r"""//ti
            set_type('Root', {
                d: 'dict<int:any>'
            });
            .to_type('Root');
            range(-11111, 11111, 11).each(|i| .d[i] = `key {i}`);
        """)
        res = await client.query('.d;')
        self.assertEqual(sorted(res),
                         [[i, f'key {i}'] for i in range(-11111, 11111, 11)])

        self.assertEqual((await client.query('.d[-11111];')), 'key -11111')
        res = await client.query(r"""//ti
            s = "overwrite";
            .d[-11111] = s;
            .d[-11111] = s;  // dubble overwrite to test ref count
        """)
        self.assertEqual((await client.query('.d[-11111];')), 'overwrite')

        with self.assertRaisesRegex(
                LookupError,
                r'key `1` not found'):
            await client.query('.d[1]')

        with self.assertRaisesRegex(
                TypeError,
                r'dict key must be of type `int`, got `str`'):
            await client.query('.d["1"] = "new";')






if __name__ == '__main__':
    run_test(TestDict())
