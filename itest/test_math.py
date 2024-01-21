#!/usr/bin/env python
import asyncio
import pickle
import time
import math
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


class TestMath(TestBase):

    title = 'Test math functions'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_const(self, client):
        res = await client.query("""//ti
            [
                MATH_E,
                MATH_PI,
            ];
        """)
        self.assertEqual(res, [
            2.718281828459045,
            3.141592653589793
        ])

    async def test_abs(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `abs`'):
            await client.query('a = 0.1; a.abs();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `abs` takes 1 argument '
                r'but 0 were given'):
            await client.query('abs();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `abs` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('abs(false);')

        with self.assertRaisesRegex(
                OverflowError,
                r'integer overflow'):
            await client.query('abs(INT_MIN);')

        res = await client.query('abs(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                abs(-0.0),
                abs(-0),
                abs(inf),
                abs(-inf),
                abs(-2.1e+10000),
                abs(7.5),
                abs(-7.5),
                abs(42),
                abs(-42),
                abs(INT_MAX),
                abs(INT_MIN+1),
            ];
        """)
        self.assertEqual(res, [
            0.0,
            0,
            math.inf,
            math.inf,
            math.inf,
            7.5,
            7.5,
            42,
            42,
            9223372036854775807,
            9223372036854775807,
        ])

    async def test_ceil(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `ceil`'):
            await client.query('a = 0.1; a.ceil();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `ceil` takes 1 argument '
                r'but 0 were given'):
            await client.query('ceil();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `ceil` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('ceil(false);')

        with self.assertRaisesRegex(
                OverflowError,
                r'integer overflow'):
            await client.query('ceil(inf);')

        with self.assertRaisesRegex(
                OverflowError,
                r'integer overflow'):
            await client.query('ceil(nan);')

        with self.assertRaisesRegex(
                OverflowError,
                r'integer overflow'):
            await client.query('ceil(2.0e+100);')

        res = await client.query("""//ti
            [
                ceil(0.0),
                ceil(0),
                ceil(7.123),
                ceil(7.789),
                ceil(-7.123),
                ceil(-7.789),
            ];
        """)
        self.assertEqual(res, [
            0,
            0,
            8,
            8,
            -7,
            -7
        ])

    async def test_cos(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `cos`'):
            await client.query('a = 0.1; a.cos();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `cos` takes 1 argument '
                r'but 0 were given'):
            await client.query('cos();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `cos` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('cos(false);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('cos(inf);')

        res = await client.query('cos(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                cos(0.0),
                cos(0),
                cos(0.52),
                cos(-0.52),
                cos(1),
                cos(-1),
            ];
        """)
        self.assertEqual(res, [
            math.cos(0.0),
            math.cos(0),
            math.cos(0.52),
            math.cos(-0.52),
            math.cos(1),
            math.cos(-1),
        ])

    async def test_exp(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `exp`'):
            await client.query('a = 0.1; a.exp();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `exp` takes 1 argument '
                r'but 0 were given'):
            await client.query('exp();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `exp` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('exp(false);')

        res = await client.query('exp(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                exp(0.0),
                exp(0),
                exp(0.52),
                exp(-0.52),
                exp(3),
                exp(-3),
                exp(inf),
                exp(-inf),
            ];
        """)
        self.assertEqual(res, [
            math.exp(0.0),
            math.exp(0),
            math.exp(0.52),
            math.exp(-0.52),
            math.exp(3),
            math.exp(-3),
            math.exp(math.inf),
            math.exp(-math.inf)
        ])

    async def test_floor(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `floor`'):
            await client.query('a = 0.1; a.floor();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `floor` takes 1 argument '
                r'but 0 were given'):
            await client.query('floor();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `floor` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('floor(false);')

        with self.assertRaisesRegex(
                OverflowError,
                r'integer overflow'):
            await client.query('floor(inf);')

        with self.assertRaisesRegex(
                OverflowError,
                r'integer overflow'):
            await client.query('floor(nan);')

        with self.assertRaisesRegex(
                OverflowError,
                r'integer overflow'):
            await client.query('floor(2.0e+100);')

        res = await client.query("""//ti
            [
                floor(0.0),
                floor(0),
                floor(7.123),
                floor(7.789),
                floor(-7.123),
                floor(-7.789),
            ];
        """)
        self.assertEqual(res, [
            0,
            0,
            7,
            7,
            -8,
            -8
        ])

    async def test_log10(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `log10`'):
            await client.query('a = 0.1; a.log10();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `log10` takes 1 argument '
                r'but 0 were given'):
            await client.query('log10();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `log10` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('log10(false);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('log10(-inf);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('log10(0);')

        res = await client.query('log10(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                log10(0.1),
                log10(1),
                log10(1.2e+3),
                log10(1.2e-3),
                log10(200),
                log10(inf),
            ];
        """)
        self.assertEqual(res, [
            math.log10(0.1),
            math.log10(1),
            math.log10(1.2e+3),
            math.log10(1.2e-3),
            math.log10(200),
            math.log10(math.inf),
        ])

    async def test_log2(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `log2`'):
            await client.query('a = 0.1; a.log2();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `log2` takes 1 argument '
                r'but 0 were given'):
            await client.query('log2();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `log2` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('log2(false);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('log2(-inf);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('log2(0);')

        res = await client.query('log2(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                log2(0.1),
                log2(1),
                log2(1.2e+3),
                log2(1.2e-3),
                log2(200),
                log2(inf),
            ];
        """)
        self.assertEqual(res, [
            math.log2(0.1),
            math.log2(1),
            math.log2(1.2e+3),
            math.log2(1.2e-3),
            math.log2(200),
            math.log2(math.inf),
        ])

    async def test_loge(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `loge`'):
            await client.query('a = 0.1; a.loge();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `loge` takes 1 argument '
                r'but 0 were given'):
            await client.query('loge();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `loge` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('loge(false);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('loge(-inf);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('loge(0);')

        res = await client.query('loge(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                loge(0.1),
                loge(1),
                loge(1.2e+3),
                loge(1.2e-3),
                loge(200),
                loge(inf),
            ];
        """)
        self.assertEqual(res, [
            math.log(0.1),
            math.log(1),
            math.log(1.2e+3),
            math.log(1.2e-3),
            math.log(200),
            math.log(math.inf),
        ])

    async def test_pow(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `pow`'):
            await client.query('a = 0.1; a.pow();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `pow` takes 2 arguments '
                r'but 0 were given'):
            await client.query('pow();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `pow` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('pow(false, 0);')

        with self.assertRaisesRegex(
                TypeError,
                r'function `pow` expects argument 2 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('pow(0, false);')

        res = await client.query('pow(nan, 1);')
        self.assertTrue(math.isnan(res))

        res = await client.query('pow(0, nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query('pow(-0.1, -0.1);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                pow(0, 0),
                pow(0.0, 0.0),
                pow(2, 2),
                pow(2.34, 2.34),
                pow(inf, inf),
                pow(0, inf),
                pow(nan, 0),
                pow(1, nan),
                pow(-1.0, 2.0),
            ];
        """)
        self.assertEqual(res, [
            math.pow(0, 0),
            math.pow(0.0, 0.0),
            math.pow(2, 2),
            math.pow(2.34, 2.34),
            math.pow(math.inf, math.inf),
            math.pow(0, math.inf),
            math.pow(math.nan, 0),
            math.pow(1, math.nan),
            math.pow(-1.0, 2.0),
        ])

    async def test_sin(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `sin`'):
            await client.query('a = 0.1; a.sin();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `sin` takes 1 argument '
                r'but 0 were given'):
            await client.query('sin();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `sin` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('sin(false);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('sin(inf);')

        res = await client.query('sin(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                sin(0.0),
                sin(0),
                sin(0.52),
                sin(-0.52),
                sin(1),
                sin(-1),
            ];
        """)
        self.assertEqual(res, [
            math.sin(0.0),
            math.sin(0),
            math.sin(0.52),
            math.sin(-0.52),
            math.sin(1),
            math.sin(-1),
        ])

    async def test_sqrt(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `sqrt`'):
            await client.query('a = 0.1; a.sqrt();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `sqrt` takes 1 argument '
                r'but 0 were given'):
            await client.query('sqrt();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `sqrt` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('sqrt(false);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('sqrt(-inf);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('sqrt(-0.1);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('sqrt(-1);')

        res = await client.query('sqrt(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                sqrt(0.0),
                sqrt(0),
                sqrt(0.1),
                sqrt(1),
                sqrt(1.2e+3),
                sqrt(1.2e-3),
                sqrt(200),
                sqrt(inf),
            ];
        """)
        self.assertEqual(res, [
            math.sqrt(0.0),
            math.sqrt(0),
            math.sqrt(0.1),
            math.sqrt(1),
            math.sqrt(1.2e+3),
            math.sqrt(1.2e-3),
            math.sqrt(200),
            math.sqrt(math.inf),
        ])

    async def test_tan(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `float` has no function `tan`'):
            await client.query('a = 0.1; a.tan();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `tan` takes 1 argument '
                r'but 0 were given'):
            await client.query('tan();')

        with self.assertRaisesRegex(
                TypeError,
                r'function `sin` expects argument 1 to be of type `int` or '
                r'`float` but got type `bool` instead'):
            await client.query('sin(false);')

        with self.assertRaisesRegex(
                ValueError,
                r'math domain error'):
            await client.query('tan(inf);')

        res = await client.query('tan(nan);')
        self.assertTrue(math.isnan(res))

        res = await client.query("""//ti
            [
                tan(0.0),
                tan(0),
                tan(0.52),
                tan(-0.52),
                tan(1),
                tan(-1),
            ];
        """)
        self.assertEqual(res, [
            math.tan(0.0),
            math.tan(0),
            math.tan(0.52),
            math.tan(-0.52),
            math.tan(1),
            math.tan(-1),
        ])


if __name__ == '__main__':
    run_test(TestMath())
