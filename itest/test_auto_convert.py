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
            set_enum('Color', {
                Red: 0,
                Green: 1,
            });
            set_enum('Theme', {
                Light: 'light',
                Dark: 'dark',
            });
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
                uuid: 'uuid',
                enum: 'Color',
                senum: 'Theme',
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

        # --- General tests ---

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

        with self.assertRaisesRegex(
                ValueError,
                r"mismatch in type `T`; conversion error for "
                r"property `regex`: malformed regular expression"):
            await q("""//ti
                T({regex: '/i'});
            """)
        with self.assertRaisesRegex(
                ValueError,
                r"mismatch in type `T`; conversion error for "
                r"property `int`: cannot convert float with "
                r"fractional part to integer"):
            await q("""//ti
                T({int: 0.1});
            """)
        with self.assertRaisesRegex(
                LookupError,
                r"mismatch in type `T`; conversion error for "
                r"property `x`: collection `stuff` has no `thing` with id 0"):
            await q("""//ti
                T({x: 0});
            """)
        with self.assertRaisesRegex(
                TypeError,
                r"mismatch in type `T`; type `thing` is invalid for "
                r"property `x` with definition `&X"):
            await q("""//ti
                T({x: 1});
            """)
        with self.assertRaisesRegex(
                ValueError,
                r"mismatch in type `T`; conversion error for "
                r"property `error`: error messages should not "
                r"exceed 16383 characters, got 16384 characters"):
            await q("""//ti
                T({error: msg});
            """, msg='t'*16384)

        # --- Integer variants (int, uint, nint, pint, ranged int) ---

        await q("""//ti
            t = T({
                int: -42.0,
                uint: 0.0,
                nint: -1.0,
                pint: 5.0,
                rint: -5.0,
            });
            assert(t.int == -42);
            assert(t.uint == 0);
            assert(t.nint == -1);
            assert(t.pint == 5);
            assert(t.rint == -5);

            t_nil = T({uint: nil});
            assert(t_nil.uint == nil);
        """)

        # Non-finite floats & integer overflow
        with self.assertRaisesRegex(
                ValueError,
                r"mismatch in type `T`; conversion error for "
                r"property `int`: cannot convert non-finite float to integer"):
            await q("""//ti
                T({int: float('nan')});
            """)
        with self.assertRaisesRegex(
                ValueError,
                r"mismatch in type `T`; conversion error for "
                r"property `int`: cannot convert non-finite float to integer"):
            await q("""//ti
                T({int: float('inf')});
            """)
        with self.assertRaisesRegex(
                OverflowError,
                r"mismatch in type `T`; conversion error for "
                r"property `int`: integer overflow"):
            await q("""//ti
                T({int: 1.0e+30});
            """)

        # Range / sign violations after float -> int conversion
        with self.assertRaisesRegex(
                ValueError,
                r"mismatch in type `T`; property `uint` only accepts integer "
                r"values greater than or equal to 0"):
            await q("""//ti
                T({uint: -1.0});
            """)
        with self.assertRaisesRegex(
                ValueError,
                r"mismatch in type `T`; property `nint` only accepts negative "
                r"integer values"):
            await q("""//ti
                T({nint: 0.0});
            """)
        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; property `pint` only accepts "
                    r"positive integer values"):
            await q("""//ti
                T({pint: 0.0});
            """)
        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; property `rint` requires an "
                    r"integer value between -5 and 5 \(both inclusive\)"):
            await q("""//ti
                T({rint: 6.0});
            """)
        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; property `rint` requires an "
                    r"integer value between -5 and 5 \(both inclusive\)"):
            await q("""//ti
                T({rint: -6.0});
            """)

        # Disallowed types for int (e.g. bool, str)
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `bool` is invalid for "
                    r"property `int` with definition `int`"):
            await q("""//ti
                T({int: true});
            """)
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `str` is invalid for "
                    r"property `int` with definition `int`"):
            await q("""//ti
                T({int: "123"});
            """)

        # --- Float & ranged float ---

        await q("""//ti
            t = T({
                float: 7,
                rfloat: -5,
            });
            assert(type(t.float) == 'float');
            assert(t.float == 7.0);
            assert(type(t.rfloat) == 'float');
            assert(t.rfloat == -5.0);
        """)

        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; property `rfloat` requires a "
                    r"float value between -5.000000 and "
                    r"5.000000 \(both inclusive\)"):
            await q("""//ti
                T({rfloat: 6});
            """)
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `str` is invalid for "
                    r"property `float` with definition `float`"):
            await q("""//ti
                T({float: "1.5"});
            """)

        # --- Timeval ---

        await q("""//ti
            t1 = T({tv: 1795214029});
            assert(type(t1.tv) == 'timeval');
            assert(t1.tv == timeval(1795214029));

            t2 = T({tv: 1795214029.5});
            assert(type(t2.tv) == 'timeval');
            assert(t2.tv == timeval(1795214029.5));
        """)

        with self.assertRaisesRegex(
                OverflowError,
                r"mismatch in type `T`; conversion error for "
                r"property `tv`: date/time overflow"):
            await q("""//ti
                T({tv: float('nan')});
            """)
        with self.assertRaisesRegex(
                TypeError,
                r"mismatch in type `T`; type `str` is invalid for "
                r"property `tv` with definition `timeval`"):
            await q("""//ti
                T({tv: "2026-11-20T22:32:28Z"});
            """)

        # --- Regex ---

        await q("""//ti
            t = T({regex: '/^hello.*/i'});
            assert(type(t.regex) == 'regex');
            assert(t.regex.test('HELLO world'));
        """)

        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `int` is invalid for "
                    r"property `regex` with definition `regex`"):
            await q("""//ti
                T({regex: 123});
            """)

        # --- Error ---

        await q("""//ti
            t = T({error: 'something went wrong'});
            assert(type(t.error) == 'error');
            assert(t.error.msg() == 'something went wrong');
            assert(t.error.code() == -100);
        """)

        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `int` is invalid for "
                    r"property `error` with definition `error"):
            await q("""//ti
                T({error: 123});
            """)

        # --- Closure ---

        await q("""//ti
            t = T({func: '|a, b| a + b'});
            assert(type(t.func) == 'closure');
            assert(t.func(2, 3) == 5);
        """)

        with self.assertRaisesRegex(
                    SyntaxError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `func`: invalid syntax in closure"):
            await q("""//ti
                T({func: '|a, b| a +'});
            """)
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `int` is invalid for "
                    r"property `func` with definition `closure`"):
            await q("""//ti
                T({func: 123});
            """)

        # --- UUID ---

        await q("""//ti
            u = uuid('550e8400-e29b-41d4-a716-446655440000');

            t_str = T({uuid: '550e8400-e29b-41d4-a716-446655440000'});
            assert(type(t_str.uuid) == 'uuid');
            assert(t_str.uuid == u);

            t_bytes = T({uuid: bytes(u)});
            assert(type(t_bytes.uuid) == 'uuid');
            assert(t_bytes.uuid == u);
        """)

        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `uuid`: invalid UUID string length"):
            await q("""//ti
                T({uuid: 'not-a-valid-uuid'});
            """)
        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `uuid`: invalid UUID \(expecting 16 bytes\)"):
            await q("""//ti
                T({uuid: bytes('short')});
            """)
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `int` is invalid for "
                    r"property `uuid` with definition `uuid`"):
            await q("""//ti
                T({uuid: 123});
            """)

        # --- Enum (by name first, then by value) ---

        await q("""//ti
            t_name = T({enum: 'Green'});
            assert(t_name.enum == Color{Green});

            t_val = T({enum: 0});
            assert(t_val.enum == Color{Red});

            t_str_name = T({senum: 'Light'});
            assert(t_str_name.senum == Theme{Light});

            t_str_val = T({senum: 'dark'});
            assert(t_str_val.senum == Theme{Dark});
        """)

        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `enum`: enum `Color` is expecting a value of "
                    r"type `int` but got type `str` instead"):
            await q("""//ti
                T({enum: 'Unknown'});
            """)
        with self.assertRaisesRegex(
                    LookupError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `enum`: enum `Color` has no member with "
                    r"value 99"):
            await q("""//ti
                T({enum: 99});
            """)
        with self.assertRaisesRegex(
                    LookupError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `senum`: enum `Theme` has no member with "
                    r"value `unknown`"):
            await q("""//ti
                T({senum: 'unknown'});
            """)
        # bug #479.
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `enum`: enum `Color` is expecting a value "
                    r"of type `int` but got type `bool` instead"):
            await q("""//ti
                T({enum: true});
            """)

        # --- Typed thing & array/set element conversion errors ---

        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `tx`: mismatch in type `X`; conversion "
                    r"error for property `dt`: invalid date/time "
                    r"string \(does not match format `%Y-%m-%d`\)"):
            await q("""//ti
                T({tx: {a: {dt: 'not-a-date'}}});
            """)
        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `arr`: mismatch in type `X`; conversion "
                    r"error for property `dt`: invalid date/time "
                    r"string \(does not match format `%Y-%m-%d`\)"):
            await q("""//ti
                T({arr: [{dt: 'not-a-date'}]});
            """)
        with self.assertRaisesRegex(
                    ValueError,
                    r"mismatch in type `T`; conversion error for "
                    r"property `set`: mismatch in type `X`; conversion "
                    r"error for property `dt`: invalid date/time "
                    r"string \(does not match format `%Y-%m-%d`\)"):
            await q("""//ti
                T({set: [{dt: 'not-a-date'}]});
            """)
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `str` is invalid for "
                    r"property `set` with definition `&{X}`"):
            await q("""//ti
                T({set: 'not-an-array'});
            """)

        # --- .assign() auto-conversion & strict literal T{...} check ---

        await q("""//ti
            t = T{};
            t.assign({
                int: 10.0,
                float: 3,
                dt: dt,
                tv: 1795214029,
                regex: '/abc/',
                error: 'boom',
                func: '|x| x * 2',
                enum: 'Green',
                set: [{dt:,}],
            });
            assert(t.int == 10);
            assert(t.float == 3.0);
            assert(t.dt == datetime(dt));
            assert(t.tv == timeval(1795214029));
            assert(t.regex.test('abc'));
            assert(t.error.msg() == 'boom');
            assert(t.func(4) == 8);
            assert(t.enum == Color{Green});
            assert(is_set(t.set) && t.set.len() == 1);
        """, dt=dt)

        # Direct literal initialization must remain strict (no auto-conversion)
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `float` is invalid for "
                    r"property `int` with definition `int`"):
            await q("""//ti
                T{int: 10.0};
            """)
        with self.assertRaisesRegex(
                    TypeError,
                    r"mismatch in type `T`; type `str` is invalid for "
                    r"property `enum` with definition `Color`"):
            await q("""//ti
                T{enum: 'Green'};
            """)


if __name__ == '__main__':
    run_test(TestAutoConvert())
