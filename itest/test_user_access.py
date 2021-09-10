#!/usr/bin/env python
import asyncio
import time
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import ValueError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import AuthError
from thingsdb.exceptions import ForbiddenError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import BadDataError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import ZeroDivisionError
from thingsdb.exceptions import OperationError


class TestUserAccess(TestBase):

    title = 'Test create/modify/delete users and grant/revoke access'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        await asyncio.sleep(3)

        with self.assertRaisesRegex(AuthError, 'invalid username or password'):
            await get_client(
                self.node0, auth=['test1', 'test'], auto_reconnect=False)

        client = await get_client(self.node0)

        await client.query(r'''
            new_user("test1");
            new_user("test2");
            set_password("test1", "test");
            new_collection("junk");
            del_collection("stuff");
        ''')

        token1 = await client.query(r'''
            new_token('test2');
        ''')

        testcl1 = await get_client(
            self.node0,
            auth=['test1', 'test'],
            auto_reconnect=False)

        testcl2 = await get_client(
            self.node0,
            auth=token1,
            auto_reconnect=False)

        error_msg = 'user .* is missing the required privileges'
        now = time.time()

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''new_collection('Collection');''')

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''.map(||nil);''', scope='@:junk')

        # Deprecation `READ` privileges
        await client.query(r'''
            grant('@thingsdb', "test1", QUERY|GRANT);
            grant('@collection:junk', 'test1', READ);
        ''')

        await testcl1.query(r'''
            new_collection('Collection');
            grant('@c:Collection', 'admin', QUERY|GRANT);
            grant('@:Collection', 'test2', QUERY);
        ''')
        await testcl1.query(r'''.x = 42;''', scope='@:Collection')
        await testcl1.query(r'''.map(||nil);''', scope='@:junk')
        self.assertEqual(await testcl2.query('.x', scope='@:Collection'), 42)

        with self.assertRaisesRegex(
                OperationError,
                'it is not possible to revoke your own `GRANT` privileges'):
            await testcl1.query(
                r'''revoke('@:Collection', 'test1', CHANGE);''')

        await client.query(
            r'''revoke('@:Collection', 'test1', MODIFY);''')  # Deprecation

        users_access = await testcl1.query(r'''user_info('admin');''')

        self.assertTrue(isinstance(users_access["created_at"], int))

        # at least one info should be checked for a correct created_at info
        self.assertGreater(users_access['created_at'], now - 60)
        self.assertLess(users_access['created_at'], now + 60)
        created_at = users_access['created_at']

        self.assertEqual(users_access, {
            'access': [{
                'privileges': 'FULL',
                'scope': '@node'
            }, {
                'privileges': 'FULL',
                'scope': '@thingsdb'
            }, {
                'privileges': 'FULL',
                'scope': '@collection:junk'
            }, {
                'privileges': 'QUERY|CHANGE|GRANT',
                'scope': '@collection:Collection'
            }],
            'has_password': True,
            'created_at': created_at,
            'name': 'admin',
            'tokens': [],
            'user_id': 1
        })

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''nodes_info();''', scope='@node')

        await client.query(r'''
            grant('@node', "test1", QUERY);
        ''')

        await testcl1.query(r'''nodes_info();''', scope='@node')

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''reset_counters();''', scope='@node')

        await client.query(r'''grant('@n', "test1", EVENT);''')

        await testcl1.query(r'''reset_counters();''', scope='@node')

        await client.query(r'''del_user('test1');''')

        # queries should no longer work
        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''.map(||nil);''', scope='@:junk')

        # should not be possible to create a new client
        with self.assertRaisesRegex(AuthError, 'invalid username or password'):
            await get_client(
                self.node0, auth=['test1', 'test'], auto_reconnect=False)

        testcl1.close()
        client.close()
        await testcl1.wait_closed()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestUserAccess())
