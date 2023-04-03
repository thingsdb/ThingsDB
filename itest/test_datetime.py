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

    async def _test_datetime_now(self, client):
        res = await client.query(r'''
            [
                int(datetime()),
                int(now()),
            ];
        ''')
        self.assertAlmostEqual(res[0], res[1], delta=1)

    async def _test_datetime_err(self, client):
        with self.assertRaisesRegex(
                ValueError,
                'name `timeval` is reserved'):
            await client.query('new_type("timeval");')
        with self.assertRaisesRegex(
                ValueError,
                'name `datetime` is reserved'):
            await client.query('set_type("datetime", {});')

    async def _test_is_datetime(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_datetime` takes 1 argument but 2 were given'):
            await client.query('is_datetime(1, 2);')

        self.assertFalse(await client.query('is_datetime( err() ); '))
        self.assertTrue(await client.query('is_datetime( datetime() ); '))
        self.assertFalse(await client.query('is_datetime( timeval() ); '))

    async def _test_is_datetime(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_timeval` takes 1 argument but 2 were given'):
            await client.query('is_timeval(1, 2);')

        self.assertFalse(await client.query('is_timeval( err() ); '))
        self.assertFalse(await client.query('is_timeval( datetime() ); '))
        self.assertTrue(await client.query('is_timeval( timeval() ); '))

    async def _test_datetime(self, client):
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
                r'\(does not match format `%Y`\)'):
            await client.query('datetime("abcd");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 1 to be of '
                r'type `str` \(when called with two arguments\) but '
                r'got type `nil` instead'):
            await client.query('datetime(nil, "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 2 to be of '
                r'type `str` \(when called with two arguments\) but '
                r'got type `nil` instead'):
            await client.query('datetime("x", nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string \(too large\)'):
            await client.query('datetime(s, "%Y");', s='x'*130)

        with self.assertRaisesRegex(
                ValueError,
                r'date/time format is restricted to a length '
                r'between 2 and 80 characters'):
            await client.query('datetime("2013", fmt);', fmt='x'*130)

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string '
                r'\(does not match format `%Y`\)'):
            await client.query('datetime("2013X", fmt);', fmt='%Y')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string '
                r'\(does not match format `%YX`\)'):
            await client.query('datetime("2013", fmt);', fmt='%YX')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 1 to be of '
                r'type `int` \(when called with three or more arguments\) '
                r'but got type `str` instead'):
            await client.query('datetime("2013", 1, 1);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 2 to be of '
                r'type `int` \(when called with three or more arguments\) '
                r'but got type `str` instead'):
            await client.query('datetime(2013, "1", 1);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 3 to be of '
                r'type `int` \(when called with three or more arguments\) '
                r'but got type `str` instead'):
            await client.query('datetime(2013, 1, "1");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 4 to be of '
                r'type `int` or type `str` \(when called with three or more '
                r'arguments\) but got type `nil` instead'):
            await client.query('datetime(2013, 1, 1, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects argument 5 to be of '
                r'type `int` or type `str` \(when called with three or more '
                r'arguments\) but got type `nil` instead'):
            await client.query('datetime(2013, 1, 1, 1, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `datetime` expects the last, and only the last '
                r'argument to be of type `str` \(when called with three '
                r'or more arguments\)'):
            await client.query('datetime(2013, 1, 1, "UTC", 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'year 0 is out of range \[1..9999\]'):
            await client.query('datetime(0, 1, 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'month 0 is out of range \[1..12\]'):
            await client.query('datetime(2013, 0, 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'day 0 is out of range \[1..31\]'):
            await client.query('datetime(2013, 12, 0);')

        with self.assertRaisesRegex(
                ValueError,
                r'hour 24 is out of range \[0..23\]'):
            await client.query('datetime(2013, 12, 31, 24);')

        with self.assertRaisesRegex(
                ValueError,
                r'minute -1 is out of range \[0..59\]'):
            await client.query('datetime(2013, 12, 31, 0, -1);')

        with self.assertRaisesRegex(
                ValueError,
                r'second -1 is out of range \[0..59\]'):
            await client.query('datetime(2013, 12, 31, 13, 0, -1);')

        with self.assertRaisesRegex(
                ValueError,
                r'unknown time zone'):
            await client.query('datetime(2013, 12, 31, 13, "XXX");')

        with self.assertRaisesRegex(
                ValueError,
                r'day 31 is out of range for month'):
            await client.query('datetime(2013, 2, 31, 13);')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid offset; expecting format `\+/-hh\[mm\]`, '
                r'for example \+0100 or -05'):
            await client.query('datetime(2013, 2, 6, "+13");')

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

        self.assertEqual(
            await client.query('datetime("2013");'),
            '2013-01-01T00:00:00Z')

        self.assertEqual(
            await client.query('datetime(2013, 1, 1);'),
            '2013-01-01T00:00:00Z')

        self.assertEqual(
            await client.query('datetime("2013-02");'),
            '2013-02-01T00:00:00Z')

        self.assertEqual(
            await client.query('datetime(2013, 2, 1);'),
            '2013-02-01T00:00:00Z')

        self.assertEqual(
            await client.query('datetime("2013-02-06");'),
            '2013-02-06T00:00:00Z')

        self.assertEqual(
            await client.query('datetime(2013, 2, 6);'),
            '2013-02-06T00:00:00Z')

        self.assertEqual(
            await client.query('datetime("2013-02-06 13");'),
            '2013-02-06T13:00:00Z')

        self.assertEqual(
            await client.query('datetime(2013, 2, 6, 13);'),
            '2013-02-06T13:00:00Z')

        self.assertEqual(
            await client.query('datetime("2013-02-06 13:12");'),
            '2013-02-06T13:12:00Z')

        self.assertEqual(
            await client.query('datetime(2013, 2, 6, 13, 12);'),
            '2013-02-06T13:12:00Z')

        self.assertEqual(
            await client.query('datetime("2013-02-06 13:12:50");'),
            '2013-02-06T13:12:50Z')

        self.assertEqual(
            await client.query('datetime(2013, 2, 6, 13, 12, 50);'),
            '2013-02-06T13:12:50Z')

        self.assertEqual(
            await client.query('datetime("2013-02-06 12:12:50+01");'),
            '2013-02-06T12:12:50+0100')

        self.assertEqual(
            await client.query('datetime(2013, 2, 6, 12, 12, 50, "+01");'),
            '2013-02-06T12:12:50+0100')

        self.assertEqual(
            await client.query(
                'datetime(2013, 2, 6, 12, 12, 50, "Europe/Amsterdam");'),
            '2013-02-06T12:12:50+0100')

        self.assertEqual(
            await client.query('datetime("2013-02-06T12:12:50+0100");'),
            '2013-02-06T12:12:50+0100')

        self.assertEqual(
            await client.query('datetime("2013/02/06 13h", "%Y/%m/%d %Hh");'),
            '2013-02-06T13:00:00Z')

    async def _test_timeval(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `timeval` takes at most 7 arguments '
                'but 8 were given'):
            await client.query('timeval(1, 2, 3, 4, 5, 6, 7, 8);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `nil` to `timeval`'):
            await client.query('timeval(nil);')

        with self.assertRaisesRegex(
                OverflowError,
                'date/time overflow'):
            await client.query('timeval(0.9e+28);')

        with self.assertRaisesRegex(
                ValueError,
                'invalid date/time string'):
            await client.query('timeval("xxx");')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid offset; expecting format `\+/-hh\[mm\]'):
            await client.query('timeval("2013-02-06T13:00:00+28");')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string '
                r'\(does not match format `%Y`\)'):
            await client.query('timeval("abcd");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `timeval` expects argument 1 to be of '
                r'type `str` \(when called with two arguments\) but '
                r'got type `nil` instead'):
            await client.query('timeval(nil, "x");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `timeval` expects argument 2 to be of '
                r'type `str` \(when called with two arguments\) but '
                r'got type `nil` instead'):
            await client.query('timeval("x", nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string \(too large\)'):
            await client.query('timeval(s, "%Y");', s='x'*130)

        with self.assertRaisesRegex(
                ValueError,
                r'date/time format is restricted to a length '
                r'between 2 and 80 characters'):
            await client.query('timeval("2013", fmt);', fmt='x'*130)

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string '
                r'\(does not match format `%Y`\)'):
            await client.query('timeval("2013X", fmt);', fmt='%Y')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid date/time string '
                r'\(does not match format `%YX`\)'):
            await client.query('timeval("2013", fmt);', fmt='%YX')

        with self.assertRaisesRegex(
                TypeError,
                r'function `timeval` expects argument 1 to be of '
                r'type `int` \(when called with three or more arguments\) '
                r'but got type `str` instead'):
            await client.query('timeval("2013", 1, 1);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `timeval` expects argument 2 to be of '
                r'type `int` \(when called with three or more arguments\) '
                r'but got type `str` instead'):
            await client.query('timeval(2013, "1", 1);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `timeval` expects argument 3 to be of '
                r'type `int` \(when called with three or more arguments\) '
                r'but got type `str` instead'):
            await client.query('timeval(2013, 1, "1");')

        with self.assertRaisesRegex(
                TypeError,
                r'function `timeval` expects argument 4 to be of '
                r'type `int` or type `str` \(when called with three or more '
                r'arguments\) but got type `nil` instead'):
            await client.query('timeval(2013, 1, 1, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `timeval` expects argument 5 to be of '
                r'type `int` or type `str` \(when called with three or more '
                r'arguments\) but got type `nil` instead'):
            await client.query('timeval(2013, 1, 1, 1, nil);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `timeval` expects the last, and only the last '
                r'argument to be of type `str` \(when called with three '
                r'or more arguments\)'):
            await client.query('timeval(2013, 1, 1, "UTC", 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'year 0 is out of range \[1..9999\]'):
            await client.query('timeval(0, 1, 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'month 0 is out of range \[1..12\]'):
            await client.query('timeval(2013, 0, 1);')

        with self.assertRaisesRegex(
                ValueError,
                r'day 0 is out of range \[1..31\]'):
            await client.query('timeval(2013, 12, 0);')

        with self.assertRaisesRegex(
                ValueError,
                r'hour 24 is out of range \[0..23\]'):
            await client.query('timeval(2013, 12, 31, 24);')

        with self.assertRaisesRegex(
                ValueError,
                r'minute -1 is out of range \[0..59\]'):
            await client.query('timeval(2013, 12, 31, 0, -1);')

        with self.assertRaisesRegex(
                ValueError,
                r'second -1 is out of range \[0..59\]'):
            await client.query('timeval(2013, 12, 31, 13, 0, -1);')

        with self.assertRaisesRegex(
                ValueError,
                r'unknown time zone'):
            await client.query('timeval(2013, 12, 31, 13, "XXX");')

        with self.assertRaisesRegex(
                ValueError,
                r'day 31 is out of range for month'):
            await client.query('timeval(2013, 2, 31, 13);')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid offset; expecting format `\+/-hh\[mm\]`, '
                r'for example \+0100 or -05'):
            await client.query('timeval(2013, 2, 6, "+13");')

        self.assertEqual(
            await client.query('timeval(1360155600);'),
            1360155600)

        self.assertEqual(
            await client.query('timeval(1360155600.123);'),
            1360155600)

        self.assertEqual(
            await client.query('timeval(datetime(1360155600));'),
            1360155600)

        self.assertEqual(
            await client.query('timeval(timeval(1360155600));'),
            1360155600)

        self.assertEqual(
            await client.query('int(timeval(1360155600));'),
            1360155600)

        self.assertEqual(
            await client.query('timeval("2013");'),
            1356998400)

        self.assertEqual(
            await client.query('timeval(2013, 1, 1);'),
            1356998400)

        self.assertEqual(
            await client.query('timeval("2013-02");'),
            1359676800)

        self.assertEqual(
            await client.query('timeval(2013, 2, 1);'),
            1359676800)

        self.assertEqual(
            await client.query('timeval("2013-02-06");'),
            1360108800)

        self.assertEqual(
            await client.query('timeval(2013, 2, 6);'),
            1360108800)

        self.assertEqual(
            await client.query('timeval("2013-02-06 13");'),
            1360155600)

        self.assertEqual(
            await client.query('timeval(2013, 2, 6, 13);'),
            1360155600)

        self.assertEqual(
            await client.query('timeval("2013-02-06 13:12");'),
            1360156320)

        self.assertEqual(
            await client.query('timeval(2013, 2, 6, 13, 12);'),
            1360156320)

        self.assertEqual(
            await client.query('timeval("2013-02-06 13:12:50");'),
            1360156370)

        self.assertEqual(
            await client.query('timeval(2013, 2, 6, 13, 12, 50);'),
            1360156370)

        self.assertEqual(
            await client.query('timeval("2013-02-06 12:12:50+01");'),
            1360149170)

        self.assertEqual(
            await client.query('timeval(2013, 2, 6, 12, 12, 50, "+01");'),
            1360149170)

        self.assertEqual(
            await client.query(
                'timeval(2013, 2, 6, 12, 12, 50, "Europe/Amsterdam");'),
            1360149170)

        self.assertEqual(
            await client.query('timeval("2013-02-06T12:12:50+0100");'),
            1360149170)

        self.assertEqual(
            await client.query('timeval("2013/02/06 13h", "%Y/%m/%d %Hh");'),
            1360155600)

    async def _test_set_time_zone(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `set_time_zone` takes 2 arguments '
                'but 0 were given'):
            await client.query('set_time_zone();', scope='@t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `set_time_zone` is undefined in the `@collection` '
                r'scope; you might want to query the `@thingsdb` scope\?'):
            await client.query('set_time_zone("stuff", "Europe/Kyiv");')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting a scope to be of type `str` but got '
                r'type `nil` instead'):
            await client.query('set_time_zone(nil, "");', scope='@t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `set_time_zone` expects argument 2 to be of '
                r'type `str` but got type `nil` instead'):
            await client.query('set_time_zone("stuff", nil);', scope='@t')

        with self.assertRaisesRegex(
                ValueError,
                'invalid scope; scopes must not be empty;'):
            await client.query('set_time_zone("", "UTC");', scope='@t')

        with self.assertRaisesRegex(
                ValueError,
                'unknown time zone'):
            await client.query('set_time_zone("stuff", "+01");', scope='@t')

        self.assertEqual(await client.query(r'''
            set_time_zone("stuff", "Europe/Kyiv");
        ''', scope='@t'), None)

        self.assertEqual(
            await client.query('datetime("2013-02-06T13:00");'),
            "2013-02-06T13:00:00+0200")

        self.assertEqual(
            await client.query('timeval("2013-02-06T13:00");'),
            1360148400)

        self.assertEqual(
            await client.query('datetime(2013, 2, 6, 13);'),
            "2013-02-06T13:00:00+0200")

        self.assertEqual(
            await client.query('timeval(2013, 2, 6, 13);'),
            1360148400)

    async def _test_time_zones_info(self, client):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `time_zones_info` takes 0 arguments '
                'but 1 was given'):
            await client.query('time_zones_info(nil);', scope='@t')

        res = await client.query('time_zones_info();', scope='@t')
        self.assertEqual(len(res), 596)

        for tz in res:
            self.assertIsInstance(tz, str)

    async def _test_extract(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `extract` takes at most 1 argument '
                'but 2 were given'):
            await client.query('.dt.extract(1, 2);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `extract` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('.dt.extract(1);')

        with self.assertRaisesRegex(
                ValueError,
                r'function `extract` is expecting argument 1 to be `year`, '
                r'`month`, `day`, `hour`, `minute`, `second` or `gmt_offset`'):
            await client.query('.dt.extract("X");')

        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `extract`'):
            await client.query('nil.extract();')

        self.assertEqual(await client.query('.dt.extract();'), {
            'day': 6,
            'gmt_offset': 3600,
            'hour': 13,
            'minute': 0,
            'month': 2,
            'second': 0,
            'year': 2013
        })

        self.assertEqual(await client.query('.dt.extract("year");'), 2013)

    async def _test_format(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `format` takes 1 argument '
                'but 2 were given'):
            await client.query('.dt.format("%Y", 2);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `format` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('.dt.format(1);')

        with self.assertRaisesRegex(
                ValueError,
                r'date/time format is restricted to a length '
                r'between 2 and 80 characters'):
            await client.query('.dt.format("");')

        with self.assertRaisesRegex(
                ValueError,
                r'date/time format is restricted to a length '
                r'between 2 and 80 characters'):
            await client.query('.dt.format(fmt);', fmt='x'*200)

        self.assertEqual(
            await client.query('.dt.format("%Y");'),
            "2013")

    async def _test_move(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `move` takes 2 arguments '
                'but 0 were given'):
            await client.query('.dt.move();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `move` expects argument 1 to be of '
                r'type `str` but got type `int` instead'):
            await client.query('.dt.move(1, 1);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `move` expects argument 2 to be of '
                r'type `int` but got type `float` instead'):
            await client.query('.dt.move("days", 1.0);')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid unit, expecting one of `years`, `months`, '
                r'`weeks`, `days`, `hours`, `minutes` or `seconds`'):
            await client.query('.dt.move("X", 0);')

        self.assertEqual(
            await client.query('.dt.move("weeks", 200);'),
            "2016-12-07T13:00:00+0100")

        self.assertEqual(
            await client.query('.dt.move("weeks", -200);'),
            "2009-04-08T14:00:00+0200")

        self.assertEqual(
            await client.query('.dt.move("days", 23);'),
            "2013-03-01T13:00:00+0100")

        self.assertEqual(
            await client.query('datetime(2020, 1, 31).move("months", 1);'),
            "2020-02-29T00:00:00Z")

        self.assertEqual(
            await client.query('datetime(2020, 2, 29).move("years", -1);'),
            "2019-02-28T00:00:00Z")

    async def _test_replace(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `replace` takes 1 argument '
                'but 0 were given'):
            await client.query('.dt.replace();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `replace` expects argument 1 to be of '
                r'type `thing` but got type `nil` instead'):
            await client.query('.dt.replace(nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'day 31 is out of range for month'):
            await client.query(r'.dt.replace({day: 31});')

        with self.assertRaisesRegex(
                TypeError,
                r'day must be of type `int` but got type `nil` instead'):
            await client.query(r'.dt.replace({day: nil});')

        with self.assertRaisesRegex(
                ValueError,
                r'year 0 is out of range \[1..9999]'):
            await client.query(r'.dt.replace({year: 0});')

        with self.assertRaisesRegex(
                ValueError,
                r'month 13 is out of range \[1..12]'):
            await client.query(r'.dt.replace({month: 13});')

        with self.assertRaisesRegex(
                ValueError,
                r'day 0 is out of range \[1..31]'):
            await client.query(r'.dt.replace({day: 0});')

        with self.assertRaisesRegex(
                ValueError,
                r'hour -1 is out of range \[0..23]'):
            await client.query(r'.dt.replace({hour: -1});')

        with self.assertRaisesRegex(
                ValueError,
                r'minute 60 is out of range \[0..59]'):
            await client.query(r'.dt.replace({minute: 60});')

        with self.assertRaisesRegex(
                ValueError,
                r'second 60 is out of range \[0..59]'):
            await client.query(r'.dt.replace({second: 60});')

        self.assertEqual(
            await client.query(r'.dt.replace({day: 15, other: nil});'),
            "2013-02-15T13:00:00+0100")

        self.assertEqual(
            await client.query(r'.dt.replace({hour: 15});'),
            "2013-02-06T15:00:00+0100")

        self.assertEqual(
            await client.query(r'''.dt.replace({
                year: 1978,
                month: 8,
                day: 7,
                hour: 15,
                minute: 28,
                second: 30,
            });'''),
            "1978-08-07T16:28:30+0200")

    async def _test_to(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `to` takes 1 argument '
                'but 0 were given'):
            await client.query('.dt.to();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `to` expects argument 1 to be of '
                r'type `str` but got type `nil` instead'):
            await client.query('.dt.to(nil);')

        with self.assertRaisesRegex(
                ValueError,
                r'expecting a time zone or offset; example time '
                r'zone: `Europe/Amsterdam`; example offset: `\+01`'):
            await client.query('.dt.to("");')

        with self.assertRaisesRegex(
                ValueError,
                r'unknown time zone'):
            await client.query('.dt.to("XXX");')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid offset; expecting format `\+/-hh\[mm]`, '
                r'for example \+0100 or -05'):
            await client.query('.dt.to("-0199");')

        self.assertEqual(
            await client.query('.dt.to("-05");'),
            "2013-02-06T07:00:00-0500")

        self.assertEqual(
            await client.query('.dt.to("Europe/Kyiv");'),
            "2013-02-06T14:00:00+0200")

    async def _test_week(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `week` takes 0 arguments '
                'but 2 were given'):
            await client.query('.dt.week(1, 2);')

        with self.assertRaisesRegex(
                LookupError,
                r'type `nil` has no function `week`'):
            await client.query('nil.week();')

        self.assertEqual(
            await client.query('datetime(2020, 1, 1).week();'), 0)

        self.assertEqual(
            await client.query('datetime(2020, 1, 7).week();'), 1)

    async def _test_weekday(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `weekday` takes 0 arguments '
                'but 2 were given'):
            await client.query('.dt.weekday(1, 2);')

        with self.assertRaisesRegex(
                LookupError,
                r'type `nil` has no function `weekday`'):
            await client.query('nil.weekday();')

        self.assertEqual(
            await client.query('datetime(2020, 1, 1).weekday();'), 3)

        self.assertEqual(
            await client.query('datetime(2020, 1, 19).weekday();'), 0)

    async def _test_yday(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `yday` takes 0 arguments '
                'but 2 were given'):
            await client.query('.dt.yday(1, 2);')

        with self.assertRaisesRegex(
                LookupError,
                r'type `nil` has no function `yday`'):
            await client.query('nil.yday();')

        self.assertEqual(
            await client.query('datetime(2020, 1, 1).yday();'), 0)

        self.assertEqual(
            await client.query('datetime(2020, 12, 8).yday();'), 342)

    async def _test_zone(self, client):
        await client.query(
            '.dt = datetime(2013, 2, 6, 13, "Europe/Amsterdam");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `zone` takes 0 arguments '
                'but 2 were given'):
            await client.query('.dt.zone(1, 2);')

        with self.assertRaisesRegex(
                LookupError,
                r'type `nil` has no function `zone`'):
            await client.query('nil.zone();')

        self.assertEqual(
            await client.query('datetime(2020, 1, 1).zone();'), 'UTC')

        self.assertEqual(
            await client.query('datetime(2020, 1, 1, "+01").zone();'), None)

        self.assertEqual(
            await client.query('.dt.zone();'), 'Europe/Amsterdam')

    async def _test_all_time_zones(self, client):
        # bug #267
        res = await client.query(r"""//ti
            is_time_zone('America/Argentina/ComodRivadavia');
        """)
        self.assertTrue(res)

    async def test_move_month_tz(self, client):
        # bug #340
        res = await client.query(r"""//ti
            datetime(int(datetime('2023-01-01')) + 3600)
                .to('-01')
                .move('months', 1)
                .move('months', 1);
        """)
        self.assertEqual(res, "2023-03-01T00:00:00-0100")

        res = await client.query(r"""//ti
            datetime(int(datetime('2023-01-01')) - 3600)
                .to('+01')
                .move('months', 1)
                .move('months', 1);
        """)
        self.assertEqual(res, "2023-03-01T00:00:00+0100")

        res = await client.query(r"""//ti
            datetime(int(datetime('2022-12-01')) + 3600)
                .to('Atlantic/Cape_Verde')
                .move('months', 1)
                .move('months', 1)
                .move('months', 1)
                .move('months', 1);  // Cape_verde = UTC-1
        """)
        self.assertEqual(res, "2023-04-01T00:00:00-0100")

        res = await client.query(r"""//ti
            datetime(int(datetime('2022-12-01')) - 3600)
                .to('Europe/Amsterdam')
                .move('months', 1)
                .move('months', 1)
                .move('months', 1)
                .move('months', 1);
        """)
        self.assertEqual(res, "2023-04-01T00:00:00+0200")

        # bug #342
        res = await client.query(r"""//ti
            datetime(int(datetime('2023-04-01')) - 7200)
                .to('Europe/Amsterdam')
                .move('months', -4);
        """)
        self.assertEqual(res, "2022-12-01T00:00:00+0100")

        res = await client.query(r"""//ti
            datetime(int(datetime('2022-12-01')) + 3600)
                .to('Atlantic/Cape_Verde')
                .move('days', 121);  // Cape_verde = UTC-1
        """)
        self.assertEqual(res, "2023-04-01T00:00:00-0100")

        res = await client.query(r"""//ti
            datetime(int(datetime('2022-12-01')) - 3600)
                .to('Europe/Amsterdam')
                .move('days', 121);
        """)
        self.assertEqual(res, "2023-04-01T00:00:00+0200")

        res = await client.query(r"""//ti
            datetime(int(datetime('2022-12-01')) + 3600)
                .to('Atlantic/Cape_Verde')
                .move('weeks', 17)
                .move('days', 2);  // Cape_verde = UTC-1
        """)
        self.assertEqual(res, "2023-04-01T00:00:00-0100")

        res = await client.query(r"""//ti
            datetime(int(datetime('2022-12-01')) - 3600)
                .to('Europe/Amsterdam')
                .move('weeks', 17)
                .move('days', 2);
        """)
        self.assertEqual(res, "2023-04-01T00:00:00+0200")


if __name__ == '__main__':
    run_test(TestDatetime())
