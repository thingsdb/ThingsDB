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
            await get_client(self.node0, username='test', password='test')

        client = await get_client(self.node0)

        await client.query(r'''
            new_user("test");
            set_password("test", "test");
            new_collection("junk");
            del_collection("stuff");
        ''')

        testcl = await get_client(
            self.node0,
            username='test',
            password='test',
            auto_reconnect=False)

        error_msg = 'user .* is missing the required privileges'

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl.query(r'''new_collection('some_collection');''')

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl.query(r'''map(||nil);''', target='junk')

        await client.query(r'''
            grant(':thingsdb', "test", GRANT);
            grant('junk', 'test', READ);
        ''')

        await testcl.query(r'''
            new_collection('some_collection');
            grant('some_collection', 'admin', GRANT);
        ''')

        await testcl.query(r'''x = 1;''', target='some_collection')
        await testcl.query(r'''map(||nil);''', target='junk')

        with self.assertRaisesRegex(
                BadRequestError,
                'it is not possible to revoke your own `GRANT` privileges'):
            await testcl.query(
                r'''revoke('some_collection', 'test', MODIFY);''')

        await client.query(r'''revoke('some_collection', 'test', MODIFY);''')

        users_access = await testcl.query(r'''users();''')
        self.assertEqual(users_access, [{
            'access': [{
                'privileges': 'FULL',
                'target': ':node'
            }, {
                'privileges': 'FULL',
                'target': ':thingsdb'
            }, {
                'privileges': 'FULL',
                'target': 'junk'
            }, {
                'privileges': 'READ|MODIFY|GRANT',
                'target': 'some_collection'
            }],
            'name': 'admin',
            'user_id': 1
        }, {
            'access': [{
                'privileges': 'READ|MODIFY|GRANT',
                'target': ':thingsdb'
            }, {
                'privileges': 'READ',
                'target': 'junk'
            }, {
                'privileges': 'READ|WATCH',
                'target': 'some_collection'
            }],
            'name': 'test',
            'user_id': 4
        }])

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl.query(r'''nodes();''', target=scope.node)

        await client.query(r'''
            grant(':node', "test", READ);
        ''')

        await testcl.query(r'''nodes();''', target=scope.node)

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl.query(r'''reset_counters();''', target=scope.node)

        # scope:node should work, as long as it ends with :node
        await client.query(r'''
            grant('scope:node', "test", MODIFY);
        ''')

        await testcl.query(r'''reset_counters();''', target=scope.node)

        await client.query(r'''del_user('test');''')

        # queries should no longer work
        with self.assertRaisesRegex(ForbiddenError, error_msg):
            await testcl.query(r'''map(||nil);''', target='junk')

        # should not be possible to create a new client
        with self.assertRaisesRegex(AuthError, 'invalid username or password'):
            await get_client(self.node0, username='test', password='test')

        testcl.close()
        client.close()
        await testcl.wait_closed()
        await client.wait_closed()


if __name__ == '__main__':
    run_test(TestUserAccess())
