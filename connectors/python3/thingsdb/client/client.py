import asyncio
import logging
import msgpack
import weakref
import random
import ssl
from .package import Package
from .buildin import Buildin
from .protocol import Proto
from .protocol import PROTOMAP
from .protocol import Protocol
from .protocol import proto_unkown
from ..convert import convert
from .events import Events


class _ReconnectEv(Events):

    async def on_reconnect(self):
        if self.client._reconnect:
            await self.client.watch(scope='@n')

    def on_node_status(self, status):
        if status == 'SHUTTING_DOWN':
            asyncio.ensure_future(
                self.client.reconnect(),
                loop=self.client._loop)

    def on_warning(self, warn):
        logging.warn(f'{warn["warn_msg"]} ({warn["warn_code"]})')

    def on_watch_init(self, data):
        pass

    def on_watch_update(self, data):
        pass

    def on_watch_delete(self, data):
        pass


class Client(Buildin):

    MAX_RECONNECT_WAIT_TIME = 120
    MAX_RECONNECT_TIMEOUT = 10

    def __init__(self, auto_reconnect=True, loop=None, ssl=None):
        self._loop = loop if loop else asyncio.get_event_loop()
        self._auth = None
        self._pool = None
        self._protocol = None
        self._requests = {}
        self._pid = 0
        self._reconnect = auto_reconnect
        self._scope = '@t'  # default to thingsdb scope
        self._pool_idx = 0
        self._reconnecting = False
        self._event_handlers = []
        self._ssl = ssl
        if auto_reconnect:
            self.add_event_handler(_ReconnectEv(self))

    def add_event_handler(self, event_handler):
        self._event_handlers.append(event_handler)

    def get_event_loop(self) -> asyncio.AbstractEventLoop:
        return self._loop

    def is_connected(self) -> bool:
        return bool(self._protocol and self._protocol.transport)

    async def connect_pool(self, pool, auth) -> None:
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
        ], )
        ```
        """
        assert self.is_connected() is False

        self._pool = tuple((
            (address, 9200) if isinstance(address, str) else address
            for address in pool))
        self._auth = self._auth_check(auth)
        self._pool_idx = random.randint(0, len(pool) - 1)
        await self.reconnect()

    @staticmethod
    def _auth_check(auth):
        assert ((
            isinstance(auth, (list, tuple)) and
            len(auth) == 2 and
            isinstance(auth[0], str) and
            isinstance(auth[1], str)
        ) or (
            isinstance(auth, str)
        )), 'expecting an auth token string or [username, password]'
        return auth

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
                port=port,
                ssl=self._ssl)
            _, self._protocol = await asyncio.wait_for(
                conn,
                timeout=timeout)
            self._protocol.on_event_received = self._on_event_received
            self._protocol.on_response_received = self._on_response_received
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

        for event_handler in self._event_handlers:
            await event_handler.on_reconnect()

    def get_num_things(self):
        return len(self._things)

    async def wait_closed(self):
        if self._protocol and self._protocol.close_future:
            await self._protocol.close_future

    async def authenticate(self, *auth, timeout=5):
        if len(auth) == 1:
            auth = auth[0]
        self._auth = self._auth_check(auth)
        await self._authenticate(timeout)

        if self._reconnect:
            await self.watch(scope='@n')

    async def _authenticate(self, timeout):
        future = self._write_package(
            Proto.REQ_AUTH,
            data=self._auth,
            timeout=timeout)
        await future

    def use(self, scope: str):
        if not scope.startswith('@'):
            self._scope = \
                f'@{scope}' if scope.startswith(':') else f'@:{scope}'
        else:
            self._scope = scope

    def get_scope(self):
        return self._scope

    async def query(self, query: str, scope=None, timeout=None,
                    convert_args=True, **kwargs):
        if scope is None:
            scope = self._scope

        if kwargs:
            if convert_args:
                kwargs = {k: convert(v) for k, v in kwargs.items()}
            data = [scope, query, kwargs]
        else:
            data = [scope, query]

        future = self._write_package(Proto.REQ_QUERY, data, timeout=timeout)
        return await future

    async def run(self, procedure: str, *args, scope=None, convert_args=True):
        if scope is None:
            scope = self._scope

        arguments = (convert(arg) for arg in args) if convert_args else args

        future = self._write_package(
            Proto.REQ_RUN,
            [scope, procedure, *arguments],
            timeout=None)

        return await future

    def watch(self, *ids, scope=None):
        if scope is None:
            scope = self._scope

        return self._write_package(Proto.REQ_WATCH, [scope, *ids])

    def unwatch(self, *ids: int, scope=None):
        if scope is None:
            scope = self._scope

        return self._write_package(Proto.REQ_UNWATCH, [scope, *ids])

    def _on_event_received(self, pkg):
        for event_handler in self._event_handlers:
            event_handler._evmap.get(pkg.tp)(pkg.data)

    def _on_response_received(self, pkg):
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

        data = data if is_bin else b'' if data is None else \
            msgpack.packb(data, use_bin_type=True)

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
