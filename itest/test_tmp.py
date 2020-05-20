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


class TestTmp(TestBase):

    title = 'Test type'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=100)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        # add another node for query validation
        # await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_mod_arr(self, client):
        await client.query(r'''
            set_type('TColor', {
                name: 'str',
                code: 'str'
            });
            set_enum('Color', {
                RED: TColor{
                    name: 'red',
                    code: 'ff0000'
                },
                GREEN: TColor{
                    name: 'red',
                    code: '00ff00'
                },
                BLUE: TColor{
                    name: 'blue',
                    code: '0000ff'
                }
            });
            set_type('Brick', {
                part_nr: 'int',
                color: 'Color',
            });

            set_type('Brick2', {
                part_nr: 'int',
                color: 'thing',
            });



            set_type('_Name', {
                name: 'str'
            });

            set_type('_ColorName', {
                color: '_Name'
            });
        ''')

        # self.assertEqual(len(brick_color_names), 1)
        # for brick in brick_color_names:
        #     self.assertIn('#', brick)
        #     self.assertIn('color', brick)
        #     self.assertEqual(len(brick), 2)
        #     color = brick['color']
        #     self.assertIn('#', color)
        #     self.assertIn('name', color)
        #     self.assertEqual(len(color), 2)


if __name__ == '__main__':
    run_test(TestTmp())
