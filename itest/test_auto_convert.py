#!/usr/bin/env python
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.client import Client
from thingsdb.room import Room
from thingsdb.exceptions import AssertionError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import MaxQuotaError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import OverflowError
from thingsdb.exceptions import SyntaxError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import ValueError


class TestAutoConvert(TestBase):

    title = 'Test auto conversion'

    @default_test_setup(num_nodes=1, seed=1, threshold_full_storage=10)
    async def async_run(self):
        self.maxDiff = 1000
        self.node0.version()

        await self.node0.init_and_run()

        client = await get_client(self.node0)
        client.set_default_scope('//stuff')

        await self.run_tests(client)

        client.close()
        await client.wait_closed()

    async def test_auto_conversion(self, client: Client):
        q = client.query
        dt, xid = await q("""//ti
            set_type('X', {
                dt: 'datetime',
                n: 'str',
                i: 'int',
            });
            set_type('T', {
                id: '#',
                int: 'int',
                uint: 'uint?',
                nint: 'nint',
                pint: 'pint',
                rint: 'int<-5:5>',
                float: 'float',
                rfloat: 'float<-5:5>',
                dt: 'datetime',
                tv: 'timeval',
                regex: 'regex',
                error: 'error',
                func: 'closure',
                arr: '&[X]?',
                set: '&{X}',
                x: '&X',
                tx: 'thing<X>',
            });
            set_type('I', {
                id: '#',
                t: '&T',
            });
            set_type('P', {
                id: '#',
                i: '&I',
            });
            .x = X{n: 'stored_x'};
            [datetime(), .x.id()];
        """)
        with self.assertRaisesRegex(
                ValueError,
                r"mismatch in type `X`; conversion error for "
                r"property `dt`: invalid date/time string"):
            await q("""//ti
                X({dt: ""});
            """)
        with self.assertRaisesRegex(
                TypeError,
                r"mismatch in type `X`; type `int` is invalid for "
                r"property `dt` with definition `datetime`"):
            await q("""//ti
                X({dt: 123});
            """)

        await q("""//ti
            p = {i: {t: {set: [{dt:,}]}}};
            v = P({p});
            assert(is_array(p.i.t.set));
            assert(is_set(v.i.t.set));
            assert(v.i.t.set.len() == 1);
            assert(v.i.t.set.one() != p.i.t.set[0]);
        """, dt=dt)

        await q("""//ti
            p = {i: {t: {arr: [{dt:,}]}}};
            v = P({p});
            assert(is_array(p.i.t.arr));
            assert(is_array(v.i.t.arr));
            assert(v.i.t.arr.len() == 1);
            assert(v.i.t.arr[0] != p.i.t.arr[0]);
        """, dt=dt)

        await q("""//ti
            p = {i: {t: {tx: {a: {dt:,}}}}};
            v = P({p});
            assert(is_thing(p.i.t.tx));
            assert(is_thing(v.i.t.tx));
            assert(p.i.t.tx.restriction() == nil);
            assert(v.i.t.tx.restriction() == 'X');
            assert(v.i.t.tx.a != p.i.t.tx.a);
            assert(p.i.t.tx.a.dt == dt);
            assert(v.i.t.tx.a.dt == datetime(dt));
        """, dt=dt)

        await q("""//ti
            p = {i: {t: {x: xid}}};
            v = P({p});
            assert(type(p.i.t.x) == 'int');
            assert(v.i.t.x.n == 'stored_x');
        """, xid=xid)

        await q("""//ti
            p = {i: {t: {x: xid}}};
            v = P({p});
            assert(type(p.i.t.x) == 'float');
            assert(v.i.t.x.n == 'stored_x');
        """, xid=float(xid))

if __name__ == '__main__':
    run_test(TestAutoConvert())
