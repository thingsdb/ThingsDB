#!/usr/bin/env python
import asyncio
import os
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import OperationError


script_dir = os.path.dirname(__file__)


class TestRecovery(TestBase):

    title = 'Test auto recovery'

    @default_test_setup(num_nodes=3, seed=2)
    async def async_run(self):
        await self.node0.init_and_run()

        cl0 = await get_client(self.node0)
        cl0.set_default_scope('//stuff')

        await self.node1.join_until_ready(cl0)
        await self.node2.join_until_ready(cl0)

        cl1 = await get_client(self.node1)
        cl2 = await get_client(self.node2)

        for i, c in enumerate('abcdefghijklmnop'):
            await cl0.query("""//ti
                      .set(c, i);
            """, i=i, c=c)

        await asyncio.sleep(1.0)

        await self.node0.shutdown()
        fn = os.path.join(script_dir,
                          'testdir',
                          'tdb0',
                          'store',
                          '00000000002',
                          'gcprops.mp')
        with open(fn, 'wb') as fp:
            fp.write(b'<<>>')  # force corruption

        await self.node0.run(auto_rebuild=True)

        cl0 = await get_client(self.node0)

        for cl in (cl0, cl1, cl2):
            res = await cl.query('.d;', scope='//stuff')
            self.assertEqual(res, 3)

        await cl0.close_and_wait()
        await cl1.close_and_wait()
        await cl2.close_and_wait()


if __name__ == '__main__':
    run_test(TestRecovery())
