import asyncio
import logging
import msgpack
import weakref
import random
import ssl
from deprecation import deprecated
from .package import Package
from .buildin import Buildin
from .protocol import Proto
from .protocol import Protocol
from .protocol import proto_unkown
from ..convert import convert
from .abc.events import Events


class Client(Buildin):

    MAX_RECONNECT_WAIT_TIME = 120
    MAX_RECONNECT_TIMEOUT = 10

    def __init__(self, auto_reconnect=True, loop=None, ssl=None):
        self._loop = loop if loop else asyncio.get_event_loop()
        self._auth = None
        self._pool = None
        self._protocol = None
        self._pid = 0
        self._reconnect = auto_reconnect
        self._scope = '@t'  # default to thingsdb scope
        self._pool_idx = 0
        self._reconnecting = False
        self._ssl = ssl
        self._event_handlers = []

    def add_event_handler(self, event_handler: Events) -> None:
        self._event_handlers.append(event_handler)

    def get_event_loop(self) -> asyncio.AbstractEventLoop:
        return self._loop

    def is_connected(self) -> bool:
        return bool(self._protocol and self._protocol.transport)

    def set_default_scope(self, scope: str) -> None:
        assert scope.startswith('@') or scope.startswith('/')
        self._scope = scope

    @deprecated(details='Use `set_default_scope` instead')
    def use(self, scope):
        if not scope.startswith('@'):
            scope = f'@{scope}' if scope.startswith(':') else f'@:{scope}'
        self.set_default_scope(scope)

    def get_default_scope(self):
        return self._scope

    @deprecated(details='Use `get_default_scope` instead')
    def get_scope(self):
        return self._scope

    def close(self):
        if self._protocol and self._protocol.transport:
            self._reconnect = False
            self._protocol.transport.close()

    def connection_info(self):
        if not self.is_connected():
            return 'disconnected'
        socket = self._protocol.transport.get_extra_info('socket', None)
        if socket is None:
            return 'unknown_addr'
        addr, port = socket.getpeername()
        return f'{addr}:{port}'

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

    async def connect(self, host, port=9200, timeout=5) -> None:
        assert self.is_connected() is False
        self._pool = ((host, port),)
        self._pool_idx = 0
        await self._connect(timeout=timeout)

    async def reconnect(self):
        if self._reconnecting:
            return

        self._reconnecting = True
        try:
            await self._reconnect_loop()
        finally:
            self._reconnecting = False

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

    async def query(self, query: str, scope=None, timeout=None,
                    convert_args=True, **kwargs):
        if self._protocol is None:
            raise ConnectionError('no connection')

        if scope is None:
            scope = self._scope

        if kwargs:
            if convert_args:
                kwargs = {k: convert(v) for k, v in kwargs.items()}
            data = [scope, query, kwargs]
        else:
            data = [scope, query]

        future = self._protocol.write(Proto.REQ_QUERY, data, timeout=timeout)
        return await future

    async def run(self, procedure: str, *args, scope=None, convert_args=True):
        if self._protocol is None:
            raise ConnectionError('no connection')

        if scope is None:
            scope = self._scope

        arguments = (convert(arg) for arg in args) if convert_args else args

        future = self._protocol.write(
            Proto.REQ_RUN,
            [scope, procedure, *arguments],
            timeout=None)

        return await future

    def watch(self, *ids, scope=None):
        if self._protocol is None:
            raise ConnectionError('no connection')

        if scope is None:
            scope = self._scope

        return self._protocol.write(Proto.REQ_WATCH, [scope, *ids])

    def unwatch(self, *ids: int, scope=None):
        if self._protocol is None:
            raise ConnectionError('no connection')

        if scope is None:
            scope = self._scope

        return self._protocol.write(Proto.REQ_UNWATCH, [scope, *ids])

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

    async def _connect(self, timeout=5):
        host, port = self._pool[self._pool_idx]
        try:
            conn = self._loop.create_connection(
                lambda: Protocol(
                    on_connection_lost=self._on_connection_lost,
                    on_event=self._on_event,
                    loop=self._loop),
                host=host,
                port=port,
                ssl=self._ssl)
            _, self._protocol = await asyncio.wait_for(
                conn,
                timeout=timeout)
        finally:
            self._pool_idx += 1
            self._pool_idx %= len(self._pool)

    def _on_event(self, pkg):
        if pkg.tp == Proto.ON_NODE_STATUS and \
                self._reconnect and \
                pkg.data == 'SHUTTING_DOWN':
            asyncio.ensure_future(self.reconnect(), loop=self._loop)

        for event_handler in self._event_handlers:
            event_handler(pkg.tp, pkg.data)

    def _on_connection_lost(self, protocol, exc):
        if self._protocol is not protocol:
            return

        self._protocol = None

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

        if self._reconnect:
            await self.watch(scope='@n')

        for event_handler in self._event_handlers:
            event_handler.on_reconnect()

    async def _authenticate(self, timeout):
        if self._protocol is None:
            raise ConnectionError('no connection')

        await self._protocol.write(
            Proto.REQ_AUTH,
            data=self._auth,
            timeout=timeout)
