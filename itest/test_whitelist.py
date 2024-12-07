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
from thingsdb.exceptions import ForbiddenError
from thingsdb.exceptions import ZeroDivisionError
from thingsdb.exceptions import OperationError


TFS = 200


class TestWhitelist(TestBase):

    title = 'Test whitelist'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=TFS)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_whitelist(self, client):
        q = client.query
        r = client.run

        with self.assertRaisesRegex(
                LookupError,
                'whitelist not found'):
            await q('whitelist_del("admin", "rooms");', scope='/t')

        await q("""//ti
            user = user_info().load().name;
            whitelist_add(user, "procedures", /^math_.*/);
            whitelist_add(user, "procedures", "sum");
            whitelist_add(user, "rooms", "app");
            whitelist_add(user, "rooms", /^str_.*/);
        """, scope='/t')

        room_ids = await q("""//ti
            new_procedure('sum', |a, b| a + b);
            new_procedure('mul', |a, b| a * b);
            new_procedure('math_sum', |a, b| a + b);
            new_procedure('math_mul', |a, b| a * b);
            rval = [
                .app_room = room(),
                .web_room = room(),
                .no_name_room = room(),
                .str_room_lower = room(),
                .str_room_upper = room(),
            ];
            .app_room.set_name('app');
            .web_room.set_name('web');
            .str_room_lower.set_name('str_lower');
            .str_room_upper.set_name('str_upper');
            rval.map_id();
        """)

        res = await r('sum', 3, 4)
        self.assertEqual(res, 7)

        res = await r('math_mul', 3, 4)
        self.assertEqual(res, 12)

        res = await client._join(*room_ids)
        self.assertEqual(
            res,
            [room_ids[0], None, None, room_ids[3], room_ids[4]])

        #
        # Error testing
        #
        with self.assertRaisesRegex(
                LookupError,
                r'function `whitelist_add` is undefined in the `@collection` '
                r'scope; you might want to query the `@thingsdb` scope\?;'):
            await q('whitelist_add("admin", "rooms", "a");', scope='//stuff')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `whitelist_add` requires at least 2 arguments '
                r'but 0 were given;'):
            await q('whitelist_add();', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `whitelist_add` takes at most 3 arguments '
                r'but 4 were given;'):
            await q('whitelist_add(nil, nil, nil, nil);', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `whitelist_add` expects argument 1 to be of '
                r'type `str` but got type `nil` instead;'):
            await q('whitelist_add(nil, "rooms");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                'user `no_user` not found'):
            await q('whitelist_add("no_user", "rooms");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                'whitelist already exists'):
            await q('whitelist_add("admin", "rooms");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                'app already in whitelist'):
            await q('whitelist_add("admin", "rooms", "app");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'/\^str_\.\*/ already in whitelist'):
            await q('whitelist_add("admin", "rooms", /^str_.*/);', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `whitelist_add` expects argument 2 to be of '
                r'type `str` but got type `nil` instead;'):
            await q('whitelist_add("admin", nil, "a");', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid whitelist; expecting "rooms" or "procedures'):
            await q('whitelist_add("admin", "123", "a");', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'whitelist is expecting type `str` or `regex` but got '
                r'type `float` instead'):
            await q('whitelist_add("admin", "rooms", 1.3);', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'whitelist is expecting either a regular expression or a '
                r'string according the naming rules;'):
            await q('whitelist_add("admin", "rooms", "123");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `whitelist_del` is undefined in the `@collection` '
                r'scope; you might want to query the `@thingsdb` scope\?;'):
            await q('whitelist_del("admin", "rooms", "a");', scope='//stuff')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `whitelist_del` requires at least 2 arguments '
                r'but 0 were given;'):
            await q('whitelist_del();', scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `whitelist_del` takes at most 3 arguments '
                r'but 4 were given;'):
            await q('whitelist_del(nil, nil, nil, nil);', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `whitelist_del` expects argument 1 to be of '
                r'type `str` but got type `nil` instead;'):
            await q('whitelist_del(nil, "rooms");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                'user `no_user` not found'):
            await q('whitelist_del("no_user", "rooms");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                'a not found in whitelist'):
            await q('whitelist_del("admin", "rooms", "a");', scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'/\^notin_\.\*/ not found in whitelist'):
            await \
                q('whitelist_del("admin", "rooms", /^notin_.*/);', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `whitelist_del` expects argument 2 to be of '
                r'type `str` but got type `nil` instead;'):
            await q('whitelist_del("admin", nil, "a");', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid whitelist; expecting "rooms" or "procedures'):
            await q('whitelist_del("admin", "123", "a");', scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'whitelist is expecting type `str` or `regex` but got '
                r'type `float` instead'):
            await q('whitelist_del("admin", "rooms", 1.3);', scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'whitelist is expecting either a regular expression or a '
                r'string according the naming rules;'):
            await q('whitelist_del("admin", "rooms", "123");', scope='/t')

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for room `web`'):
            await client._join(room_ids[1])

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for `room:4`'):
            await client._join(room_ids[2])

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for `mul`'):
            await r('mul', 5, 6)

        await self.node0.shutdown()
        await self.node0.run()

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for `room:4`'):
            await client._join(room_ids[2])

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for `mul`'):
            await r('mul', 5, 6)

        res = await r('sum', 5, 6)
        self.assertEqual(res, 11)

        res = await r('math_mul', 5, 6)
        self.assertEqual(res, 30)

        wl = await q('user_info().load().whitelists;')
        self.assertEqual(wl, {
            "procedures": ['/^math_.*/', 'sum'],
            "rooms": ['app', '/^str_.*/']
        })

        await q('whitelist_del("admin", "procedures", "sum");', scope='/t')
        await \
            q('whitelist_del("admin", "procedures", /^math_.*/);', scope='/t')

        wl = await q('user_info().load().whitelists;')
        self.assertEqual(wl, {
            "procedures": [],
            "rooms": ['app', '/^str_.*/']
        })

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for `sum`'):
            await r('sum', 5, 6)

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for `math_mul`'):
            await r('math_mul', 5, 6)

        # Force full storage
        for i in range(TFS):
            await q('.x = i;', i=i)

        await self.node0.shutdown()
        await self.node0.run()

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for `sum`'):
            await r('sum', 5, 6)

        with self.assertRaisesRegex(
                ForbiddenError,
                r'no match in whitelist for `math_mul`'):
            await r('math_mul', 5, 6)

        await q('whitelist_del("admin", "rooms");', scope='/t')

        wl = await q('user_info().load().whitelists;')
        self.assertEqual(wl, {
            "procedures": [],
        })
        await q('whitelist_del("admin", "procedures");', scope='/t')
        wl = await q('user_info().load().whitelists;')
        self.assertEqual(wl, {})

        res = await r('mul', 5, 7)
        self.assertEqual(res, 35)
        res = await client._join(room_ids[1])
        self.assertEqual(res, [room_ids[1]])

    async def test_whitelist_access(self, client):
        q = client.query
        await q(r"""//ti
            new_user('user');
            set_password('user', 'user');
            grant('/t', "user", USER);
        """, scope='/t')

        userlient = await get_client(
            self.node0,
            auth=['user', 'user'],
            auto_reconnect=False)

        with self.assertRaisesRegex(
                ForbiddenError,
                r'user `user` is missing the required privileges '
                r'\(`GRANT`\) on scope `@thingsdb`;'):
            uq = userlient.query
            await uq('whitelist_add("admin", "rooms", "a");', scope='/t')

        with self.assertRaisesRegex(
                ForbiddenError,
                r'user `user` is missing the required privileges '
                r'\(`GRANT`\) on scope `@thingsdb`;'):
            uq = userlient.query
            await uq('whitelist_del("admin", "rooms", "a");', scope='/t')

        userlient.close()

        await self.node0.shutdown()
        await self.node0.run()

if __name__ == '__main__':
    run_test(TestWhitelist())
