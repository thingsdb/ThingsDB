#!/usr/bin/env python
import asyncio
from lib import run_test
from lib import default_test_setup
from lib.testbase import TestBase
from lib.client import get_client
from thingsdb.exceptions import ValueError
from thingsdb.exceptions import TypeError
from thingsdb.exceptions import NumArgumentsError
from thingsdb.exceptions import LookupError
from thingsdb.exceptions import OperationError
from thingsdb.exceptions import BadDataError
from thingsdb.room import Room, event
from thingsdb.client import Client


class ORoom(Room):
    def __init__(self, actions, *args, **kwargs):
        self.actions = actions
        super().__init__(*args, **kwargs)

    @event('add')
    def on_add(self, obj):
        self.actions.append(obj)


class TRoom(Room):

    def __init__(self, actions, *args, **kwargs):
        self.actions = actions
        super().__init__(*args, **kwargs)

    def on_init(self):
        self.actions.append('on_init')

    async def on_join(self):
        self.actions.append('on_join')

    def on_leave(self):
        self.actions.append('on_leave')

    @event('msg')
    def on_msg(self, msg):
        self.actions.append(msg)

    @event('ping')
    def on_ping(self):
        self.actions.append('pong')

    def on_delete(self):
        self.actions.append('on_delete')


class TestRoom(TestBase):

    title = 'Test room type'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=100)
    async def async_run(self):
        await self.node0.init_and_run()

        cl0 = await get_client(self.node0)
        cl0.set_default_scope('//stuff')

        # add more nodes for watch validation
        await self.node1.join_until_ready(cl0)
        await self.node2.join_until_ready(cl0)

        cl1 = await get_client(self.node1)
        cl1.set_default_scope('//stuff')

        cl2 = await get_client(self.node2)
        cl2.set_default_scope('//stuff')

        await self.run_tests(cl0, cl1, cl2)

        # expected no garbage collection
        for client in (cl0, cl1, cl2):
            counters = await client.query('counters();', scope='@node')
            self.assertEqual(counters['garbage_collected'], 0)
            self.assertEqual(counters['changes_failed'], 0)

            client.close()
            await client.wait_closed()

    async def test_is_room(self, cl0, cl1, cl2):
        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `is_room` takes 1 argument but 0 were given'):
            await cl0.query('is_room();')

        self.assertTrue(await cl0.query('is_room( room() ); '))
        self.assertFalse(await cl0.query('is_room( "bla" ); '))

    async def test_room_err(self, cl0, cl1, cl2):
        with self.assertRaisesRegex(
                ValueError,
                'name `room` is reserved'):
            await cl0.query('new_type("room");')

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `room` takes at most 1 argument but 2 were given;'):
            await cl0.query('room(nil, nil);')

        with self.assertRaisesRegex(
                TypeError,
                'cannot convert type `nil` to `room`'):
            await cl0.query('room(nil);')

        with self.assertRaisesRegex(
                LookupError,
                'collection `stuff` has no `room` with id 0'):
            await cl0.query('room(0);')

    async def test_rooms(self, cl0, cl1, cl2):
        room_ids = await cl0.query(r"""//ti
            .rooms = range(3).map(|| room());
            .rooms.map(|room| room.id());
        """)
        actions = []
        rooms = [TRoom(actions, id) for id in room_ids]

        await asyncio.gather(*[room.join(cl0) for room in rooms])

        await asyncio.sleep(0.5)

        await asyncio.gather(*[
            TRoom(actions, f'.rooms[{x}].id();').join(cl1)
            for x in range(3)])

        await asyncio.sleep(0.5)

        await cl0.query(r"""//ti
            .rooms.each(|room| room.emit("msg", "Hey ho"));
        """)

        await asyncio.sleep(0.5)

        for room in rooms:
            await room.emit('msg', "Let's Go!")

        await asyncio.sleep(0.5)

        self.assertEqual(len(cl0.get_rooms()), 3)
        self.assertEqual(len(cl1.get_rooms()), 3)
        self.assertEqual(len(actions), 8*3)

        await cl0.query(r"""//ti
            .del('rooms');
        """)

        await asyncio.sleep(0.5)
        self.assertEqual(len(cl0.get_rooms()), 0)  # rooms are removed
        self.assertEqual(len(cl1.get_rooms()), 0)  # rooms are removed
        self.assertEqual(actions, [
            'on_init', 'on_init', 'on_init',
            'on_join', 'on_join', 'on_join',
            'on_init', 'on_init', 'on_init',
            'on_join', 'on_join', 'on_join',
            'Hey ho', 'Hey ho', 'Hey ho',
            'Hey ho', 'Hey ho', 'Hey ho',
            "Let's Go!", "Let's Go!", "Let's Go!",
            "Let's Go!", "Let's Go!", "Let's Go!",
            'on_delete', 'on_delete', 'on_delete',
            'on_delete', 'on_delete', 'on_delete'])

    async def test_join_leave(self, cl0, cl1, cl2):
        await cl0.query(r"""//ti
            range(3).each(|i| .set(`room{i}`, room()));
        """)

        res = await cl0._join(*range(20))
        ids = [id for id in res if id is not None]
        self.assertEqual(len(ids), 3)

        res = await cl0._leave(*range(20))
        ids = [id for id in res if id is not None]
        self.assertEqual(len(ids), 3)

        await asyncio.sleep(0.5)

        res = await cl1._leave(*range(20))
        ids = [id for id in res if id is not None]
        self.assertEqual(len(ids), 3)

    async def test_object_to_room(self, cl0, cl1, cl2):
        await cl0.query(r"""//ti
            .oroom = room();
            .oroom.set_name("oroom");
        """)
        actions = []
        oroom = ORoom(actions, 'oroom')
        await oroom.join(cl0)
        await cl0.query(r"""//ti
            .x = {y: {z: 123}};
            .oroom.emit('add', .x);
            .oroom.emit(3, 'add', .x);
            .oroom.emit(3, NO_IDS, 'add', .x);
        """)

        r0 = await cl0.query('return .x;')
        r1 = await cl0.query('return .x, 3;')
        r2 = await cl0.query('return .x, 3, NO_IDS;')

        await asyncio.sleep(0.5)

        self.assertEqual([r0, r1, r2], actions)

    async def test_room_name(self, cl0, cl1, cl2):
        await cl0.query(r"""//ti
            .room_a = room();
            .room_b = room();
        """)

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `set_name` takes 1 argument but 0 were given;'):
            await cl0.query(r"""//ti
                .room_a.set_name();
            """)

        with self.assertRaisesRegex(
                NumArgumentsError,
                'function `rooom` takes 0 arguments but 1 was given;'):
            await cl0.query('.room_a.name(nil);')

        with self.assertRaisesRegex(
                TypeError,
                'function `set_name` expects argument 1 to be of '
                'type `str` or `nil` but got type `int` instead;'):
            await cl0.query('.room_a.set_name(1);')

        with self.assertRaisesRegex(
                ValueError,
                'room name must follow the naming rules;'):
            await cl0.query('.room_a.set_name("");')

        await cl0.query('.room_a.set_name("a");')
        await cl0.query('.room_b.set_name("b");')

        # same name should work
        res = await cl0.query('.room_a.set_name("a");')
        self.assertIs(res, None)

        with self.assertRaisesRegex(
                LookupError,
                'room `a` already exists'):
            await cl0.query('.room_b.set_name("a");')

        # room b is without name
        res = await cl0.query('.room_b.set_name(nil);')

        with self.assertRaisesRegex(
                LookupError,
                'collection `stuff` has no `room` with name `b`'):
            await cl0.query('room("b");')

        with self.assertRaisesRegex(
                OperationError,
                'names can only be assigned to stored '
                r'rooms \(a room with an Id\)'):
            await cl0.query('room().set_name("c");')

        res = await cl0._join("a", "b")
        self.assertTrue(isinstance(res[0], int))
        self.assertIs(res[1], None)

        res = await cl0._leave("a", "b")
        self.assertTrue(isinstance(res[0], int))
        self.assertIs(res[1], None)

        res = await cl0._emit("a", "Test")
        self.assertIs(res, None)

        with self.assertRaisesRegex(
                BadDataError,
                'emit request only accepts an integer '
                'room id or string room name;'):
            await cl0._emit(3.4, "Test")

        with self.assertRaisesRegex(
                ValueError,
                'room name must follow the naming rules;'):
            await cl0._emit("", "Test")

        with self.assertRaisesRegex(
                LookupError,
                'collection `stuff` has no `room` with name `not_here`'):
            await cl0._emit("not_here", "Test")

        with self.assertRaisesRegex(
                LookupError,
                'collection `stuff` has no `room` with id 12345'):
            await cl0._emit(12345, "Test")

        await cl0.query('.room_a.set_name("A");')

        res = await cl0.query('[.room_a.name(), .room_b.name()];')
        self.assertEqual(res[0], "A")
        self.assertIs(res[1], None)

        res = await cl0.query('room("A").name();')
        self.assertEqual(res, "A")


if __name__ == '__main__':
    run_test(TestRoom())
