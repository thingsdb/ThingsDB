#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import ForbiddenError


class TestCommits(TestBase):

    title = 'Test commit history'

    def with_node1(self):
        if hasattr(self, 'node1'):
            return True
        print('''
            WARNING: Test requires a second node!!!
        ''')

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):

        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.set_default_scope('//stuff')

        # add another node
        if hasattr(self, 'node1'):
            await self.node1.join_until_ready(client0)

        client1 = await get_client(self.node0)
        client1.set_default_scope('//stuff')

        await self.run_tests(client0.query, client1.query)

        for client in (client0, client1):
            client.close()
            await client.wait_closed()

    async def test_set_history(self, q0, q1):
        with self.assertRaisesRegex(
                LookupError,
                r'function `set_history` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` scope\?'):
            await q0("""//ti
                set_history();
            """, scope='/n')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `set_history` takes 2 arguments but 0 were given;'):
            await q0("""//ti
                set_history();
            """, scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting a scope to be of type `str` but '
                r'got type `nil` instead'):
            await q0("""//ti
                set_history(nil, true);
            """, scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid scope; scopes must not be empty'):
            await q0("""//ti
                set_history("", true);
            """, scope='/t')

        with self.assertRaisesRegex(
                OperationError,
                r'commit history is not supported by `@node` scopes'):
            await q0("""//ti
                set_history("/n", true);
            """, scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'collection `s` not found'):
            await q0("""//ti
                set_history("//s", true);
            """, scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'function `set_history` expects argument 2 to be of '
                r'type `bool` but got type `int` instead;'):
            await q0("""//ti
                set_history("//stuff", 0);
            """, scope='/t')

        await q0("""//ti
            set_history("//stuff", true);
        """, scope='/t')
        for q in (q0, q1):
            r = await q("""//ti
                        wse();
                        collection_info('stuff').load().commit_history;
                        """, scope='/t')
            self.assertEqual(r, 0)

        await q0("""//ti
            set_history("//stuff", false);
        """, scope='/t')
        for q in (q0, q1):
            r = await q("""//ti
                        wse();
                        collection_info('stuff').load().commit_history;
                        """, scope='/t')
            self.assertEqual(r, 'disabled')

        await q0("""//ti
            set_history("/t", true);
        """, scope='/t')
        for q in (q0, q1):
            r = await q("""//ti
                        wse();
                        node_info().load().commit_history;
                        """, scope='/n')
            self.assertEqual(r, 0)

        await q0("""//ti
            set_history("/t", false);
        """, scope='/t')
        for q in (q0, q1):
            r = await q("""//ti
                        wse();
                        node_info().load().commit_history;
                        """, scope='/n')
            self.assertEqual(r, 'disabled')

    async def test_access(self, q0, q1):
        await q0("""//ti
                 new_collection('junk');
                 new_user('test1');
                 set_password('test1', 'test');
                 grant('/t', 'test1', QUERY);
                 grant('//stuff', 'test1', QUERY|CHANGE);
                 grant('//junk', 'test1', QUERY);
                 """, scope='/t')

        q = (await get_client(
            self.node0,
            auth=['test1', 'test'],
            auto_reconnect=False)).query

        with self.assertRaisesRegex(
                ForbiddenError,
                r'user `test1` is missing the required '
                r'privileges \(`CHANGE`\) on scope `@thingsdb`;'):
            await q("""//ti
                set_history('//junk', true);
            """, scope='/t')

        await q0("""//ti
                 grant('/t', 'test1', QUERY|CHANGE);
                 """, scope='/t')

        with self.assertRaisesRegex(
                ForbiddenError,
                r'user `test1` is missing the required '
                r'privileges \(`QUERY\|CHANGE`\) on '
                r'scope `@collection:junk`;'):
            await q("""//ti
                set_history('//junk', true);
            """, scope='/t')
        await q("""//ti
            set_history('//stuff', true);
            set_history('/t', false);
        """, scope='/t')

    async def test_history(self, q0, q1):
        await q0("""//ti
            set_history('//stuff', false);
            set_history('/t', false);
        """, scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `history` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` scope\?'):
            await q0("""//ti
                history({last:0});
            """, scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                r'no scope found with commit history enabled;'):
            await q0("""//ti
                history({last:0});
            """, scope='/t')

        with self.assertRaisesRegex(
                OperationError,
                r'commit history is not enabled for the `/t` scope'):
            await q0("""//ti
                history({scope: '/t', last:0});
            """, scope='/t')

        with self.assertRaisesRegex(
                OperationError,
                r'commit history is not enabled for the `//stuff` scope'):
            await q0("""//ti
                history({scope: '//stuff', last:0});
            """, scope='/t')

        with self.assertRaisesRegex(
                OperationError,
                r'commit history is not supported by `@node` scopes'):
            await q0("""//ti
                history({scope: '/n', last:0});
            """, scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'collection `s` not found'):
            await q0("""//ti
                history({scope: '//s', last:0});
            """, scope='/t')

        await q0("""//ti
            set_history('//stuff', true);
            set_history('/t', true);
        """, scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'option `last` expects an integer value greater '
                r'than or equal to 0'):
            await q0("""//ti
                history({scope: '/t', last:-1});
            """, scope='/t')

        with self.assertRaisesRegex(
                TypeError,
                r'expecting `last` to be of type `int` but got '
                r'type `bool` instead'):
            await q0("""//ti
                history({scope: '/t', last:false});
            """, scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'more than one scope has commit history enabled; you must '
                r'use the `scope` option to tell ThingsDB which scope '
                r'to use;'):
            await q0("""//ti
                history({last:0});
            """, scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'at least one of the following options must be set: '
                r'`contains`, `match`, `id`, `first`, `last`, `before`, '
                r'`after`, `has_err`;'):
            await q0("""//ti
                history({scope: '//stuff'});
            """, scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid option `x` was provided; valid options are: '
                r'`scope`, `contains`, `match`, `id`, `first`, `last`, '
                r'`before`, `after`, `has_err`, `detail`'):
            await q0("""//ti
                history({scope: '//stuff', x: ''});
            """, scope='/t')

    async def test_del_history(self, q0, q1):
        await q0("""//ti
            set_history('//stuff', false);
            set_history('/t', false);
        """, scope='/t')

        with self.assertRaisesRegex(
                LookupError,
                r'function `del_history` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` scope\?'):
            await q0("""//ti
                del_history({last:0});
            """, scope='/n')

        with self.assertRaisesRegex(
                LookupError,
                r'no scope found with commit history enabled;'):
            await q0("""//ti
                del_history({last:0});
            """, scope='/t')

        await q0("""//ti
            set_history('//stuff', true);
            set_history('/t', true);
        """, scope='/t')

        with self.assertRaisesRegex(
                ValueError,
                r'invalid option `x` was provided; valid options are: '
                r'`scope`, `contains`, `match`, `id`, `first`, `last`, '
                r'`before`, `after`, `has_err`$'):
            await q0("""//ti
                del_history({scope: '//stuff', x: ''});
            """, scope='/t')

    async def test_commit(self, q0, q1):
        await q0("""//ti
            set_history('//stuff', false);
            set_history('/t', false);
        """, scope='/t')
        with self.assertRaisesRegex(
                LookupError,
                r'function `commit` is undefined in the `@node` scope; '
                r'you might want to query the `@thingsdb` or '
                r'a `@collection` scope\?'):
            await q0("""//ti
                     commit('Test');
            """, scope='/n')
        with self.assertRaisesRegex(
                OperationError,
                r'commit history is not enabled'):
            await q0("""//ti
                     commit('Test');
                     new_type('A');
            """)

        await q0("""//ti
            set_history('//stuff', true);
            set_history('/t', true);
        """, scope='/t')

        with self.assertRaisesRegex(
                NumArgumentsError,
                r'function `commit` takes 1 argument but 0 were given'):
            await q0("""//ti
                     commit();
                     new_type('A');
            """)
        with self.assertRaisesRegex(
                TypeError,
                r'function `commit` expects argument 1 to be of type `str` '
                r'but got type `int` instead'):
            await q0("""//ti
                     commit(123);
                     new_type('A');
            """)
        with self.assertRaisesRegex(
                OperationError,
                r'commit without a change;'):
            await q0("""//ti
                     commit('Test');
            """)

        for i in range(10):
            await q0("""//ti
                    commit(`Create type T{i}`);
                    set_type(`T{i}`, {name: 'str'});
            """, i=i)

        await asyncio.sleep(3.0)

        for i in range(10):
            await q0("""//ti
                    commit(`Add x to type T{i}`);
                    mod_type(`T{i}`, 'add', 'x', 'int');
            """, i=i)

        before, after, both, deleted = await q0("""//ti
                wse();
                moment = datetime().move('seconds', -2);
                [
                    history({
                        scope: '//stuff',
                        before: moment,
                    }),
                    history({
                        scope: '//stuff',
                        after: moment,
                    }),
                    history({
                        scope: '//stuff',
                        before: moment,
                        after: moment,
                    }),
                    del_history({
                        scope: '//stuff',
                        before: moment,
                        after: moment,
                     }),
                ];
                """, scope='/t')

        self.assertEqual(len(before), 10)
        self.assertEqual(len(after), 10)
        self.assertEqual(len(both), 0)
        self.assertEqual(deleted, 0)

        self.assertTrue(before[0]['id'] < after[0]['id'])

        with self.assertRaisesRegex(
                OperationError,
                r'function `to_type` requires a commit before it can be used '
                r'on the `root\(\)` of a `@collection` scope with commit '
                r'history enabled;'):
            await q0("""//ti
                     .to_type('T0');
            """)








if __name__ == '__main__':
    run_test(TestCommits())
