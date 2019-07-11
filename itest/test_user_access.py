#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import AuthError
from thingsdb.exceptions import ForbiddenError
from thingsdb.exceptions import BadRequestError
from thingsdb import scope


class TestUserAccess(TestBase):

    title = 'Test create/modify/delete users and grant/revoke access'

    @default_test_setup(num_nodes=1, seed=1)
    async def run(self):

        await self.node0.init_and_run()

        with self.assertRaisesRegex(AuthError, 'invalid username or password'):
            await get_client(self.node0, auth=['test1', 'test'])

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

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''new_collection('Collection');''')

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''map(||nil);''', target='junk')

        await client.query(r'''
            grant('.thingsdb', "test1", GRANT);
            grant('junk', 'test1', READ);
        ''')

        await testcl1.query(r'''
            new_collection('Collection');
            grant('Collection', 'admin', GRANT);
            grant('Collection', 'test2', READ);
        ''')

        await testcl1.query(r'''x = 42;''', target='Collection')
        await testcl1.query(r'''map(||nil);''', target='junk')
        self.assertEqual(await testcl2.query('x', target='Collection'), 42)

        with self.assertRaisesRegex(
                BadRequestError,
                'it is not possible to revoke your own `GRANT` privileges'):
            await testcl1.query(
                r'''revoke('Collection', 'test1', MODIFY);''')

        await client.query(r'''revoke('Collection', 'test1', MODIFY);''')

        users_access = await testcl1.query(r'''user('admin');''')
        self.assertEqual(users_access, {
            'access': [{
                'privileges': 'FULL',
                'target': '.node'
            }, {
                'privileges': 'FULL',
                'target': '.thingsdb'
            }, {
                'privileges': 'FULL',
                'target': 'junk'
            }, {
                'privileges': 'READ|MODIFY|GRANT',
                'target': 'Collection'
            }],
            'name': 'admin',
            'tokens': [],
            'user_id': 1
        })

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''nodes();''', target=scope.node)

        await client.query(r'''
            grant('.node', "test1", READ);
        ''')

        await testcl1.query(r'''nodes();''', target=scope.node)

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''reset_counters();''', target=scope.node)

        # scope.node should work, as long as it ends with `.node`
        await client.query(r'''
            grant('scope.node', "test1", MODIFY);
        ''')

        await testcl1.query(r'''reset_counters();''', target=scope.node)

        await client.query(r'''del_user('test1');''')

        # queries should no longer work
        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl1.query(r'''map(||nil);''', target='junk')

        # should not be possible to create a new client
        with self.assertRaisesRegex(AuthError, 'invalid username or password'):
            await get_client(self.node0, auth=['test1', 'test'])

        testcl1.close()
        client.close()
        await testcl1.wait_closed()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestUserAccess())
