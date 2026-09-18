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


class TestUuid(TestBase):

    title = 'Test UUID type'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_uuid(self, client):

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `uuid`'):
            await client.query('nil.uuid();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `uuid` takes at most 1 argument but 2 were given; '
                r'see https://docs.thingsdb.io/v1/collection-api/uuid'):
            await client.query('uuid(nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `nil` to `uuid`'):
            await client.query('uuid(nil);')

        with self.assertRaisesRegex(
                ValueError,
                'invalid hex character in UUID string'):
            await client.query("uuid('01G08b43-4abd-7529-82a2-9ae352b2f10f')")

        with self.assertRaisesRegex(
                ValueError,
                'invalid UUID string length'):
            await client.query("uuid('01a08b43-4abd752982a2-9ae352b2f10f')")

        u = await client.query("""//ti
            uuid();
        """)
        self.assertIsInstance(u, str)

        u = await client.query("""//ti
            type(uuid());
        """)
        self.assertEqual(u, 'uuid')

        u = await client.query("""//ti
            uuid(base64_decode("AaCLQ0q9dSmCoprjUrLxDw=="));
        """)
        self.assertEqual(u, '01a08b43-4abd-7529-82a2-9ae352b2f10f');

        u = await client.query("""//ti
            uuid('01a08b43-4abd-7529-82a2-9ae352b2f10f');
        """)
        self.assertEqual(u, '01a08b43-4abd-7529-82a2-9ae352b2f10f');

        u = await client.query("""//ti
            uuid('01A08B434ABD752982A29AE352B2F10F');
        """)
        self.assertEqual(u, '01a08b43-4abd-7529-82a2-9ae352b2f10f');

        u = await client.query("""//ti
            str(uuid('01A08B434ABD752982A29AE352B2F10F'));
        """)
        self.assertEqual(u, '01a08b43-4abd-7529-82a2-9ae352b2f10f');

        u = await client.query("""//ti
            bytes(uuid('01A08B434ABD752982A29AE352B2F10F'));
        """)
        self.assertEqual(u,
                         b'\x01\xa0\x8bCJ\xbdu)\x82\xa2\x9a\xe3R\xb2\xf1\x0f');

        u = await client.query("""//ti
            range(5000).map(|| uuid()).is_unique();
        """)
        self.assertTrue(u)

    async def test_cmp(self, client):
        u = await client.query("""//ti
            a = uuid('01a08b43-4abd-7529-82a2-9ae352b2f10f');
            b = uuid('01A08B434ABD752982A29AE352B2F10F');
            a == b;
        """)
        self.assertTrue(u)
        u = await client.query("""//ti
            a = uuid('01a08b43-4abd-7529-82a2-9ae352b2f10f');
            b = uuid('01A08B434ABD752982A29AE352B00000');
            a != b;
        """)
        self.assertTrue(u)
        u = await client.query("""//ti
            a = 4;
            b = 7;
            assert(a < b);
            assert(a <= b);
            assert(a <= a);
            assert(b > a);
            assert(b >= a);
            assert(b >= b);
            true;
        """)
        self.assertTrue(u)


if __name__ == '__main__':
    run_test(TestUuid())
