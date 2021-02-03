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
from thingsdb.exceptions import ForbiddenError


class TestFuture(TestBase):

    title = 'Test futures'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def run(self):

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_recursion(self, client):
        with self.assertRaisesRegex(
                OperationError,
                r'maximum recursion depth exceeded'):
            await client.query(r'''
                fut = || {
                    future(nil, fut).then(|_, fut| {
                        fut();
                    });
                };
                fut();
            ''')

    async def test_future_var(self, client):
        with self.assertRaisesRegex(
                TypeError,
                r'`>` not supported between `future` and `int`'):
            await client.query(r'''
                future(nil) > 1;
            ''')

        res = await client.query(r'''
            type(future(nil));
        ''')
        self.assertEqual(res, "future")

        res = await client.query(r'''
            f = future(nil);
            .x = f;  // Assign to collectoin converts the future to `nil`
            type(.x);
        ''')
        self.assertEqual(res, "nil")

        res = await client.query(r'''
            f = future(nil);
            x = f;
            type(x);
        ''')
        self.assertEqual(res, "future")

        res = await client.query(r'''
            f = future(nil);
            x = [f];
            type(x[0]);
        ''')
        self.assertEqual(res, "nil")

    async def test_future_mod_type(self, client):
        res = await client.query(r'''
            set_type('T', {
                name: 'str',
            });
            f = future(nil, T{}).then(|_, x| {
                x.age;
            });
            mod_type('T', 'add', 'age', 'int');
            f;
        ''')
        self.assertEqual(res, 0)

        res = await client.query(r'''
            set_type('Y', {
                name: 'str',
            });
            future(nil, Y{});
            type_count('Y');
        ''')
        self.assertEqual(res, 1)

    async def test_future_gc(self, client):
        await client.query(r'''
            y = {};
            y.y = y;
            future(nil, y).then(|_, y| {
               x = {};
               x.x = x;
            });
            y = nil;
            "done";
        ''')

    async def test_append_results(self, client):
        res = await client.query(r'''
            ret = [];
            future(nil, "test1", ret).then(|_, res, ret| {
                ret.push(res);
            });
            future(nil, "test2", ret).then(|_, res, ret| {
                ret.push(res);
            });
            ret;
        ''')
        self.assertEqual(sorted(res), ['test1', 'test2'])

    async def test_procedure_with_future(self, client):
        await client.query(r'''
            new_procedure('test_rx', || {
                future(nil).then(|| {
                    .x;
                });
            });
            new_procedure('test_rw', || {
                future(nil).then(|| {
                    .x = 42;
                });
            });
        ''')

        read_token, write_token = await client.query(r'''
            new_user('read');
            new_user('write');
            grant('//stuff', 'read', RUN);
            grant('//stuff', 'write', RUN|EVENT);
            [new_token('read'), new_token('write')];
        ''', scope='@t')

        error_msg = 'user .* is missing the required privileges'

        cl_read = await get_client(self.node0, auth=read_token)
        cl_read.set_default_scope('//stuff')

        cl_write = await get_client(self.node0, auth=write_token)
        cl_write.set_default_scope('//stuff')

        with self.assertRaisesRegex(ForbiddenError, error_msg):
            self.assertEqual(await cl_read.run('test_rw'), 42)

        self.assertEqual(await cl_write.run('test_rw'), 42)
        self.assertEqual(await cl_read.run('test_rx'), 42)
        self.assertEqual(await cl_read.run('test_rx'), 42)


if __name__ == '__main__':
    run_test(TestFuture())
