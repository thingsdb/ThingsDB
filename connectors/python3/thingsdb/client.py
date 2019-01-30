import asyncio
import struct
import logging
import qpack
import weakref
from .package import Package
from .protocol import Protocol
from .protocol import REQ_AUTH
from .protocol import REQ_QUERY
from .protocol import REQ_WATCH
from .protocol import PROTOMAP
from .protocol import proto_unkown
from .protocol import ON_WATCH
from .watch import WatchMixin
from .root import Root


class Client(WatchMixin, Root):
    def __init__(self, loop=None):
        self._loop = loop if loop else asyncio.get_event_loop()
        self._username = None
        self._password = None
        self._host = None
        self._port = None
        self._protocol = None
        self._requests = {}
        self._things = {}
        self._watching = weakref.WeakSet()
        self._target = 0  # root target

    def get_event_loop(self):
        return self._loop

    def is_connected(self):
        return self._protocol and self._protocol.transport

    async def connect(self, host, port=9200, timeout=5):
        self._host = host
        self._port = port
        conn = self._loop.create_connection(
            Protocol,
            host=self._host,
            port=self._port)
        _, self._protocol = await asyncio.wait_for(
            conn,
            timeout=timeout)
        self._protocol.on_package_received = self._on_package_received
        self._pid = 0

    def close(self):
        if self._protocol and self._protocol.transport:
            self._protocol.transport.close()

    def get_num_watch(self):
        return len(self._watching)

    async def wait_closed(self):
        if self._protocol and self._protocol.close_future:
            await self._protocol.close_future

    async def authenticate(self, username, password, timeout=5):
        self._username = username
        self._password = password
        future = self._write_package(
            REQ_AUTH,
            data=(self._username, self._password),
            timeout=timeout)
        await future

    def use(self, target):
        assert isinstance(target, (int, str))
        self._target = target

    async def query(
            self, query, blobs=None, target=None, timeout=None, as_list=False):
        assert isinstance(query, str)
        assert blobs is None or isinstance(blobs, (list, tuple))
        assert target is None or isinstance(target, (int, str))

        target = self._target if target is None else target
        data = {'query': query}
        if target:
            data['target'] = target
        if blobs:
            data['blobs'] = blobs
        future = self._write_package(REQ_QUERY, data, timeout=timeout)
        result = await future
        if not as_list and len(result) == 1:
            result = result[0]
        return result

    def watch(self, things, collection=None, timeout=None):
        assert collection is None or isinstance(collection, (int, str))
        ids = [t.id() for t in things]
        collection = self._target if collection is None else collection
        assert collection, 'for watching, a collection is required'
        data = {
            'things': ids,
            'collection': collection
        }
        return self._write_package(REQ_WATCH, data, timeout=timeout)

    def unwatch(self, things, collection=None, timeout=None):
        assert collection is None or isinstance(collection, (int, str))
        ids = [t.id() for t in things]
        assert collection is None or isinstance(collection, (int, str))
        collection = self._target if collection is None else collection
        assert collection, 'for un-watching, a collection is required'
        data = {
            'things': ids,
            'collection': collection
        }
        return self._write_package(REQ_UNWATCH, data, timeout=timeout)

    def _on_package_received(self, pkg):
        if pkg.tp in ON_WATCH:
            self._on_watch_received(pkg)
            return

        try:
            future, task = self._requests.pop(pkg.pid)
        except KeyError:
            logging.error('received package id not found: {}'.format(pkg.pid))
            return None

        # cancel the timeout task
        if task is not None:
            task.cancel()

        if future.cancelled():
            return

        PROTOMAP.get(pkg.tp, proto_unkown)(future, pkg.data)

    async def _timer(self, pid, timeout):
        await asyncio.sleep(timeout)
        try:
            future, task = self._requests.pop(pid)
        except KeyError:
            logging.error('timed out package id not found: {}'.format(
                    self._data_package.pid))
            return None

        future.set_exception(TimeoutError(
            'request timed out on package id {}'.format(pid)))

    def _write_package(self, tp, data=None, is_bin=False, timeout=None):
        self._pid += 1
        self._pid %= 0x10000  # pid is handled as uint16_t

        data = data if is_bin else b'' if data is None else qpack.packb(data)

        header = Package.st_package.pack(
            len(data),
            self._pid,
            tp,
            tp ^ 0xff)

        self._protocol.transport.write(header + data)

        task = asyncio.ensure_future(
            self._timer(self._pid, timeout)) if timeout else None

        future = asyncio.Future()
        self._requests[self._pid] = (future, task)
        return future
