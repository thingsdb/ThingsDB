import asyncio
import struct
import logging
import qpack
import weakref
import random
import collections
from .package import Package
from .protocol import Protocol
from .protocol import REQ_AUTH
from .protocol import REQ_QUERY_NODE
from .protocol import REQ_QUERY_THINGSDB
from .protocol import REQ_QUERY_COLLECTION
from .protocol import REQ_WATCH
from .protocol import PROTOMAP
from .protocol import proto_unkown
from .protocol import ON_WATCH
from .watch import WatchMixin
from .root import Root
from .target import Target


class Client(WatchMixin, Root):

    MAX_RECONNECT_WAIT_TIME = 120
    MAX_RECONNECT_TIMEOUT = 10

    # Target, used for Client().query(target=...)
    node = Target('scope:node', _proto=REQ_QUERY_NODE)
    thingsdb = Target('scope:thingsdb', _proto=REQ_QUERY_THINGSDB)

    def __init__(self, auto_reconnect=True, auto_watch=True, loop=None):
        self._loop = loop if loop else asyncio.get_event_loop()
        self._username = None
        self._password = None
        self._pool = None
        self._protocol = None
        self._requests = {}
        self._pid = 0
        self._reconnect = auto_reconnect
        self._things = weakref.WeakValueDictionary()
        self._watching = weakref.WeakSet()
        self._auto_watch = auto_watch
        self._target = self.thingsdb  # default target
        self._pool_idx = 0
        self._reconnecting = False

    def get_event_loop(self) -> asyncio.AbstractEventLoop:
        return self._loop

    def is_connected(self) -> bool:
        return bool(self._protocol and self._protocol.transport)

    async def connect_pool(self, pool, username, password) -> None:
        """Connect using a connection pool.

        Argument `pool` should be an iterable with node address strings, or
        with `address` and `port` combinations in a tuple or list.

        Argument `timeout` can be be used to control the maximum time
        the client will attempt to create and authenticate a connection, the
        default timeout is 5 seconds.

        Do not use this method if the client is already
        connected. This can be checked with `client.is_connected()`.

        Example:
        ```
        await connect_pool([
            'node01.local',             # address as string
            'node02.local',             # port will default to 9200
            ('node03.local', 9201),     # ..or with an eplicit/alternative port
        ])
        ```
        """
        assert self.is_connected() is False

        self._pool = tuple((
            (address, 9200) if isinstance(address, str) else address
            for address in pool))
        self._username = username
        self._password = password

        self._pool = tuple(pool)
        self._pool_idx = random.randint(0, len(pool) - 1)
        await self.reconnect()

    async def connect(self, host, port=9200, timeout=5) -> None:
        assert self.is_connected() is False
        self._pool = ((host, port),)
        self._pool_idx = 0
        await self._connect(timeout=timeout)

    async def _connect(self, timeout=5):
        host, port = self._pool[self._pool_idx]
        try:
            conn = self._loop.create_connection(
                lambda: Protocol(on_lost=self._on_lost, loop=self._loop),
                host=host,
                port=port)
            _, self._protocol = await asyncio.wait_for(
                conn,
                timeout=timeout)
            self._protocol.on_package_received = self._on_package_received
        finally:
            self._pool_idx += 1
            self._pool_idx %= len(self._pool)

    def close(self):
        if self._protocol and self._protocol.transport:
            self._reconnect = False
            self._protocol.transport.close()

    async def reconnect(self):
        if self._reconnecting:
            return

        self._reconnecting = True
        try:
            await self._reconnect_loop()
        finally:
            self._reconnecting = False

    def connection_info(self):
        if not self.is_connected():
            return 'disconnected'
        socket = self._protocol.transport.get_extra_info('socket', None)
        if socket is None:
            return 'unknown_addr'
        addr, port = socket.getpeername()
        return f'{addr}:{port}'

    def _on_lost(self, protocol, exc):
        if self._protocol is not protocol:
            return

        self._protocol = None

        if self._requests:
            logging.error(
                f'canceling {len(self._requests)} requests '
                'due to a lost connection'
            )
            while self._requests:
                _key, (future, task) = self._requests.popitem()
                if task is not None:
                    task.cancel()
                if not future.cancelled():
                    future.cancel()

        if self._reconnect:
            asyncio.ensure_future(self.reconnect(), loop=self._loop)

    async def _reconnect_loop(self):
        wait_time = 1
        timeout = 2
        protocol = self._protocol
        while True:
            host, port = self._pool[self._pool_idx]
            try:
                await self._connect(timeout=timeout)
            except Exception as e:
                logging.error(
                    f'connecting to {host}:{port} failed ({e}), '
                    f'try next connect in {wait_time} seconds'
                )
            else:
                if protocol and protocol.transport:
                    # make sure the `old` connection will be dropped
                    self._loop.call_later(10.0, protocol.transport.close)
                break

            await asyncio.sleep(wait_time)
            wait_time *= 2
            wait_time = min(wait_time, self.MAX_RECONNECT_WAIT_TIME)
            timeout = min(timeout+1, self.MAX_RECONNECT_TIMEOUT)

        await self._authenticate(timeout=5)

        # re-watch all watching things
        watch_ids = collections.defaultdict(list)
        for t in self._watching:
            watch_ids[t._collection._id].append(t)

        if watch_ids:
            for collection_id, thing_ids in watch_ids.items():
                await self.watch(things=thing_ids, target=collection_id)
        elif self._auto_watch:
            await self.watch()

    def get_num_watch(self):
        return len(self._watching)

    def get_num_things(self):
        return len(self._things)

    async def wait_closed(self):
        if self._protocol and self._protocol.close_future:
            await self._protocol.close_future

    async def authenticate(self, username, password, timeout=5):
        self._username = username
        self._password = password
        await self._authenticate(timeout)

        if self._auto_watch:
            await self.watch()

    async def _authenticate(self, timeout):
        future = self._write_package(
            REQ_AUTH,
            data=(self._username, self._password),
            timeout=timeout)
        await future

    def use(self, target):
        if isinstance(target, (int, str)):
            target = Target(target)
        assert isinstance(target, Target)
        self._target = target

    def get_target(self):
        return self._target

    def _get_target_instance(self, target):
        if isinstance(target, (int, str)):
            return Target(target)

        return self._target if target is None else target

    async def query(
                self, query: str, deep=None, all_=False, blobs=None,
                target=None, timeout=None, as_list=False):
        target = self._get_target_instance(target)

        if target.is_collection():
            data = {
                'query': query,
                'collection': target._target,
            }
            if blobs:
                data['blobs'] = blobs
            if deep is not None and deep != 1:
                data['deep'] = deep
            if all_:
                data['all'] = True
        else:
            data = {
                'query': query
            }

        future = self._write_package(target._proto, data, timeout=timeout)
        result = await future
        if all_:
            if not as_list and len(result) == 1:
                result = result[0]
        elif as_list:
            result = [result]

        return result

    def watch(self, things=[], target=None, timeout=None):
        target = self._get_target_instance(target)
        data = {
            'things': [t if isinstance(t, int) else t._id for t in things],
            'collection': target._target
        } if target.is_collection() else None
        return self._write_package(REQ_WATCH, data, timeout=timeout)

    def unwatch(self, things, target=None, timeout=None):
        target = self._get_target_instance(target)
        data = {
            'things': [t if isinstance(t, int) else t._id for t in things],
            'collection': target._target
        } if target.is_collection() else None
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
        if self._protocol is None:
            raise ConnectionError('no connection')

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
