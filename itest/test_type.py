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


class TestType(TestBase):

    title = 'Test type'

    @default_test_setup(num_nodes=2, seed=1, threshold_full_storage=100)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.use('stuff')

        # add another node for query validation
        await self.node1.join_until_ready(client)

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_set_prop(self, client):
        await client.query(r'''
            set_type('Pet', {});
            set_type('setPet', {animal: '{Pet}'});
            .t = {};
        ''')

        with self.assertRaisesRegex(
                TypeError,
                r'mismatch in type `setPet`; '
                r'property `animal` has definition `{Pet}` but '
                r'got a set with type `thing` instead'):
            await client.query(r'''
                .t.p = setPet{
                    animal: set({})
                };
            ''')

        with self.assertRaisesRegex(
                TypeError,
                r'type `thing` is not allowed in restricted set'):
            await client.query(r'''
                .t.p = setPet{
                    animal: set()
                };
                .t.p.animal.add({});
            ''')

    async def test_new_type(self, client):
        await client.query(r'''
            set_type('User', {
                name: 'str',
                age: 'uint',
                likes: '[User]?',
            });
            set_type(new_type('People'), {
                users: '[User]'
            });
            set_type(new_type('UserName'), {
                name: 'str',
            });
        ''')

        await client.query(r'''
            .iris = new('User', {
                name: 'Iris',
                age: 6,
            });
            .cato = User{
                name: 'Cato',
                age: 5,
            };
            .people = new('People', {users: [.iris, .cato]});
        ''')

    async def test_mod_type_add(self, client):
        await client.query(r'''
            set_type(new_type('User'), {
                name: 'str',
                age: 'uint',
                likes: '[User]?',
            });
            set_type('People', {
                users: '[User]'
            });
            set_type('UserName', {
                name: 'str',
                u: 'UserName?',
            });
        ''')

        await client.query(r'''
            .iris = new('User', {
                name: 'Iris',
                age: 6,
            });
            .cato = User{
                name: 'Cato',
                age: 5,
            };
            .lena = User{
                name: 'Lena',
                age: 6,
            };
            .people = new('People', {users: [.iris, .cato, .lena]});
        ''')

        await client.query(r'''
            mod_type('User', 'add', 'friend', 'User?', User{
                name: 'Anne',
                age: 5
            });
            mod_type('User', 'del', 'age');
            mod_type('User', 'mod', 'name', 'str?');
        ''')

        await asyncio.sleep(1.5)

        client1 = await get_client(self.node1)
        client1.use('stuff')

        await self.wait_nodes_ready(client)
        iris_node0 = await client.query('return(.iris, 2);')
        iris_node1 = await client1.query('return(.iris, 2);')

        client1.close()
        await client1.wait_closed()

        self.assertEqual(iris_node0, iris_node1)
        self.assertIs(iris_node0.get('age'), None)
        self.assertEqual(iris_node0.get('friend').get('name'), 'Anne')

    async def test_wrap(self, client0):
        only_name = await client0.query(r'''
            set_type('_Name', {name: 'any'});
            .only_name = {
                name: 'Iris',
                age: 6
            }.wrap('_Name');
        ''')

        self.assertEqual(only_name['name'], 'Iris')
        self.assertIn('#', only_name)

        iris = {k: v for k, v in only_name.items()}
        iris['age'] = 6

        await asyncio.sleep(1.5)

        client1 = await get_client(self.node1)
        client1.use('stuff')

        await self.wait_nodes_ready(client0)

        for client in (client0, client1):
            test = await client.query('.only_name;')
            self.assertEqual(test, only_name)

        for client in (client0, client1):
            test = await client.query('.only_name.unwrap();')
            self.assertEqual(test, iris)

        client1.close()
        await client1.wait_closed()


if __name__ == '__main__':
    run_test(TestType())
