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


class TestDatetime(TestBase):

    title = 'Test datetime (and timeval)'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        self.node0.version()

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_datetime_now(self, client):
        res = await client.query(r'''
            // modulo 2 to prevent a near miss in seconds
            [
                int(datetime()) % 2,
                int(now()) % 2
            ];
        ''')
        self.assertEqual(res[0], res[1])

    async def test_datetime_err(self, client):
        with self.assertRaisesRegex(
                ValueError,
                'name `timeval` is reserved'):
            await client.query('new_type("timeval");')
        with self.assertRaisesRegex(
                ValueError,
                'name `datetime` is reserved'):
            await client.query('set_type("datetime", {});')

    async def test_is_datetime(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_datetime` takes 1 argument but 2 were given'):
            await client.query('is_datetime(1, 2);')

        self.assertFalse(await client.query('is_datetime( err() ); '))
        self.assertTrue(await client.query('is_datetime( datetime() ); '))
        self.assertFalse(await client.query('is_datetime( timeval() ); '))

    async def test_is_datetime(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_timeval` takes 1 argument but 2 were given'):
            await client.query('is_timeval(1, 2);')

        self.assertFalse(await client.query('is_timeval( err() ); '))
        self.assertFalse(await client.query('is_timeval( datetime() ); '))
        self.assertTrue(await client.query('is_timeval( timeval() ); '))

    async def test_datetime(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `datetime` takes at most 7 arguments '
                'but 8 were given'):
            await client.query('datetime(1, 2, 3, 4, 5, 6, 7, 8);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `nil` to `datetime`'):
            await client.query('datetime(nil);')

        with self.assertRaisesRegex(
                OverflowError,
                'date/time overflow'):
            await client.query('datetime(0.9e+28);')

        with self.assertRaisesRegex(
                ValueError,
                'invalid date/time string'):
            await client.query('datetime("xxx");')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid offset; expecting format `\+/-hh\[mm\]'):
            await client.query('datetime("2013-02-06T13:00:00+28");')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string '
                r'\(does not match format string `%Y`\)'):
            await client.query('datetime("abcd");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 1 to be of '
                r'type `str` \(when called using 2 arguments\) but '
                r'got type `nil` instead'):
            await client.query('datetime(nil, "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 2 to be of '
                r'type `str` \(when called using 2 arguments\) but '
                r'got type `nil` instead'):
            await client.query('datetime("x"", nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string \(too large\)'):
            await client.query('datetime(s, "%Y");', s='x'*130)

        self.assertEqual(
            await client.query('datetime(1360155600);'),
            '2013-02-06T13:00:00Z')

        self.assertEqual(
            await client.query('datetime(1360155600.123);'),
            '2013-02-06T13:00:00Z')

        self.assertEqual(
            await client.query('datetime(datetime(1360155600));'),
            '2013-02-06T13:00:00Z')

        self.assertEqual(
            await client.query('datetime(timeval(1360155600));'),
            '2013-02-06T13:00:00Z')

        self.assertEqual(
            await client.query('int(datetime(1360155600));'),
            1360155600)

        # `is_datetime`, `is_timeval`, `datetime`, `timeval`, `set_time_zone`, `time_zones_info`, `extract`, `format`, `move`, `replace`, `to`, `week`, `weekday`, `yday` and `zone` functions.

if __name__ == '__main__':
    run_test(TestDatetime())
