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


class TestEnum(TestBase):

    title = 'Test enumerators'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_set_enum(self, client):
        with self.assertRaisesRegex(
                LookupError,
                'type `nil` has no function `set_enum`'):
            await client.query('nil.set_enum();')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `set_enum` takes 2 arguments but 0 were given'):
            await client.query('set_enum();')

        with self.assertRaisesRegex(
                TypeError,
                'function `set_enum` expects argument 1 to be of type `str` '
                'but got type `nil` instead'):
            await client.query(r'set_enum(nil, {X: 0});')

        with self.assertRaisesRegex(
                ValueError,
                'function `set_enum` expects '
                'argument 1 to be a valid enum name'):
            await client.query(r'''set_enum('=invalid', {X: 0});''')

        with self.assertRaisesRegex(
                TypeError,
                'function `set_enum` expects argument 2 to be of type `thing` '
                'but got type `nil` instead'):
            await client.query(r'''set_enum('Color', nil);''')

        self.assertIs(await client.query(r'''
            set_enum('Color', {
                RED: '#FF0000',
                GREEN: '#00FF00',
                BLUE: '#0000FF'
            });'''), None)

        with self.assertRaisesRegex(
                LookupError,
                'enum `Color` already exists'):
            await client.query(r'''set_enum('Color', {X: 1});''')



if __name__ == '__main__':
    run_test(TestEnum())
