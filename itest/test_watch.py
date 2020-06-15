#!/usr/bin/env python
import asyncio
import pickle
import time
import weakref
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
from thingsdb.client.abc.events import Events


class TestEvents(Events):

    messages = []

    def __init__(self, client):
        super().__init__()
        self.client = client
        self._things = weakref.WeakValueDictionary()  # watching these things

    def register(self, thing):
        self._things[thing._id] = thing

    def on_reconnect(self):
        pass

    def on_node_status(self, status):
        pass

    def on_warning(self, warn):
        pass

    def on_watch_init(self, data):
        thing_dict = data['thing']
        thing = self._things.get(thing_dict.pop('#'))
        assert thing

        thing.on_init(data['event'], thing_dict)

    def on_watch_update(self, data):
        id = data.pop('#')
        thing = self._things.get(id)
        assert thing, f'missing thing with id {id}'

        thing.on_update(data['event'], data.pop('jobs'))

    def on_watch_delete(self, data):
        id = data.pop('#')
        thing = self._things.get(id)
        if thing:
            thing.on_delete()

    def on_watch_stop(self, data):
        pass


class Thing:

    def __init__(self, evhandler, id):
        self._id = id
        self._evhandler = evhandler
        evhandler.register(self)

    async def watch(self):
        await self._evhandler.client.watch(self._id)

    async def unwatch(self):
        await self._evhandler.client.unwatch(self._id)

    def on_init(self, event, data):
        for k, v in data.items():
            setattr(self, k, v)

    def _job_add(self, add_job):
        raise NotImplementedError

    def _job_remove(self, remove_job):
        raise NotImplementedError

    def _job_set(self, set_job):
        for prop, val in set_job.items():
            setattr(self, prop, val)

    def _job_splice(self, pair):
        (_k, v), = pair.items()

        _index, _count, *items = v
        self.push_cb(self._evhandler, items)

    def _job_event(self, ev):
        event, *args = ev
        if (event == 'msg'):
            TestEvents.messages.append(args[0])

    def _job_del(self, job_del):
        for prop in job_del:
            delattr(self, prop)

    def on_update(self, event, jobs):
        for job_dict in jobs:
            for name, job in job_dict.items():
                jobfun = self._UPDMAP.get(name)
                if jobfun is None:
                    raise TypeError(f'unknown job `{name}` for `{self}`')
                jobfun(self, job)

    def on_delete(self):
        self._evhandler._things.pop(self._id)

    _UPDMAP = {
        'set': _job_set,
        'del': _job_del,
        'event': _job_event,
        'splice': _job_splice,
        'add': _job_add,
        'remove': _job_remove,
    }


class TestWatch(TestBase):

    title = 'Test watching thingsdb'

    @default_test_setup(num_nodes=3, seed=1, threshold_full_storage=100)
    async def run(self):
        await self.node0.init_and_run()

        client0 = await get_client(self.node0)
        client0.set_default_scope('//stuff')
        ev0 = TestEvents(client0)
        client0.add_event_handler(ev0)

        # add more nodes for watch validation
        await self.node1.join_until_ready(client0)
        await self.node2.join_until_ready(client0)

        client1 = await get_client(self.node1)
        client1.set_default_scope('//stuff')
        ev1 = TestEvents(client1)
        client1.add_event_handler(ev1)

        client2 = await get_client(self.node2)
        client2.set_default_scope('//stuff')
        ev2 = TestEvents(client2)
        client2.add_event_handler(ev2)

        await self.run_tests(ev0, ev1, ev2)

        # expected no garbage collection
        for client in (client0, client1, client2):
            counters = await client.query('counters();', scope='@node')
            self.assertEqual(counters['garbage_collected'], 0)
            self.assertEqual(counters['events_failed'], 0)

            client.close()
            await client.wait_closed()

    async def test_watch(self, ev0, ev1, ev2):
        iris = await ev0.client.query('.iris = {};')

        await asyncio.sleep(0.5)

        iris0 = Thing(ev0, iris['#'])
        iris1 = Thing(ev1, iris['#'])
        iris2 = Thing(ev2, iris['#'])

        await iris0.watch()
        await iris1.watch()
        await iris2.watch()

        await ev0.client.query('.iris.name = "Iris";')

        await asyncio.sleep(0.5)

        self.assertEqual(iris0.name, 'Iris')
        self.assertEqual(iris1.name, 'Iris')
        self.assertEqual(iris2.name, 'Iris')

        await iris0.unwatch()
        await iris1.unwatch()
        await iris2.unwatch()

    async def test_emit(self, ev0, ev1, ev2):
        iris = await ev0.client.query('.iris = {};')

        await asyncio.sleep(0.5)

        iris0 = Thing(ev0, iris['#'])
        iris1 = Thing(ev1, iris['#'])
        iris2 = Thing(ev2, iris['#'])

        await iris0.watch()
        await iris1.watch()
        await iris2.watch()

        await ev0.client.query('.iris.emit("msg", "from ev0");')
        await ev1.client.query('.iris.emit("msg", "from ev1");')
        await ev2.client.query('.iris.emit("msg", "from ev2");')

        await ev0.client.query('.iris.emit("msg", {msg: "ev0"});')
        await ev1.client.query('.iris.emit("msg", {msg: "ev1"});')
        await ev2.client.query('.iris.emit("msg", {msg: "ev2"});')

        await ev0.client.query('.iris.emit(0, "msg", {msg: "ev0"});')
        await ev1.client.query('.iris.emit(0, "msg", {msg: "ev1"});')
        await ev2.client.query('.iris.emit(0, "msg", {msg: "ev2"});')

        await asyncio.sleep(0.5)

        self.assertEqual(TestEvents.messages, [
            'from ev0',
            'from ev0',
            'from ev0',
            'from ev1',
            'from ev1',
            'from ev1',
            'from ev2',
            'from ev2',
            'from ev2',
            {"msg": "ev0"},
            {"msg": "ev0"},
            {"msg": "ev0"},
            {"msg": "ev1"},
            {"msg": "ev1"},
            {"msg": "ev1"},
            {"msg": "ev2"},
            {"msg": "ev2"},
            {"msg": "ev2"},
            {}, {}, {}, {}, {}, {}, {}, {}, {}
        ])

        await iris0.unwatch()
        await iris1.unwatch()
        await iris2.unwatch()

    async def test_missing_away_mode(self, ev0, ev1, ev2):
        store = await ev0.client.query(r'.store = {arr: []};')

        await asyncio.sleep(0.6)

        store0 = Thing(ev0, store['#'])
        store1 = Thing(ev1, store['#'])
        store2 = Thing(ev2, store['#'])

        await store0.watch()
        await store1.watch()
        await store2.watch()

        watch_list = []

        def push_cb(ev, items):
            for item in items:
                # this is the actual test, the ID must exist on the node
                # after an event with this ID is received (even in away mode)
                i = Thing(ev, item['#'])
                asyncio.ensure_future(i.watch())
                watch_list.append(i)

        store0.push_cb = push_cb
        store1.push_cb = push_cb
        store2.push_cb = push_cb

        for _ in range(120):
            await ev0.client.query(r'.store.arr.push({x: 41});')
            await asyncio.sleep(0.2)

        await ev0.client.query(r'.store.arr.each(|x| x.x += 1);')

        await asyncio.sleep(1.6)

        self.assertEqual(len(watch_list), 120 * 3)
        for i in watch_list:
            self.assertEqual(i.x, 42)

    async def test_away_mode(self, ev0, ev1, ev2):
        evmap = [ev for ev in (ev0, ev1, ev2)]
        make_iris = True
        need_check = True
        away_soon = False

        while need_check:

            if make_iris:
                iris = await ev0.client.query(r'.iris = {};')
                make_iris = False

            nodes_info = await ev0.client.query('nodes_info();', scope='@n')
            for node in nodes_info:
                if node['status'] == 'AWAY_SOON':
                    away_soon = True

                if away_soon and node['status'] == 'AWAY':
                    node_id = node['node_id']
                    await asyncio.sleep(0.1)

                    ev = evmap[node_id]

                    await ev.client.query('.iris.age = 6;')

                    await asyncio.sleep(0.5)

                    Iris = Thing(ev, iris['#'])

                    await Iris.watch()

                    await asyncio.sleep(0.5)

                    self.assertEqual(Iris.age, 6)

                    await ev.client.query('.iris.name = "Iris";')

                    await asyncio.sleep(0.5)

                    self.assertEqual(Iris.name, 'Iris')

                    ninfo = await ev0.client.query('nodes_info();', scope='@n')
                    for n in nodes_info:
                        if n['status'] == 'AWAY' and n['node_id'] == node_id:
                            need_check = False
                            break
                    else:
                        make_iris = True

                    break

            await asyncio.sleep(0.4)


if __name__ == '__main__':
    run_test(TestWatch())
