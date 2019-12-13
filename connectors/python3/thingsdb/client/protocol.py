import enum
import asyncio
import logging
import msgpack
from typing import Optional, Any, Callable
from .package import Package
from ..exceptions import AssertionError
from ..exceptions import AuthError
from ..exceptions import BadDataError
from ..exceptions import ForbiddenError
from ..exceptions import InternalError
from ..exceptions import LookupError
from ..exceptions import MaxQuotaError
from ..exceptions import MemoryError
from ..exceptions import NodeError
from ..exceptions import NumArgumentsError
from ..exceptions import OperationError
from ..exceptions import OverflowError
from ..exceptions import RequestCancelError
from ..exceptions import RequestTimeoutError
from ..exceptions import ResultTooLargeError
from ..exceptions import SyntaxError
from ..exceptions import ThingsDBError
from ..exceptions import TypeError
from ..exceptions import ValueError
from ..exceptions import WriteUVError
from ..exceptions import ZeroDivisionError


class Proto(enum.IntEnum):
    # Events
    ON_NODE_STATUS = 0x00
    ON_WATCH_INI = 0x01
    ON_WATCH_UPD = 0x02
    ON_WATCH_DEL = 0x03
    ON_WARN = 0x04

    # Responses
    RES_PING = 0x10
    RES_OK = 0x11
    RES_DATA = 0x12
    RES_ERROR = 0x13

    # Requests (initiated by the client)
    REQ_PING = 0x20
    REQ_AUTH = 0x21
    REQ_QUERY = 0x22
    REQ_WATCH = 0x23
    REQ_UNWATCH = 0x24
    REQ_RUN = 0x25


class Err(enum.IntEnum):
    """ThingsDB error codes."""

    # ThingsDB build-in errors
    EX_OPERATION_ERROR = -63
    EX_NUM_ARGUMENTS = -62
    EX_TYPE_ERROR = -61
    EX_VALUE_ERROR = -60
    EX_OVERFLOW = -59
    EX_ZERO_DIV = -58
    EX_MAX_QUOTA = -57
    EX_AUTH_ERROR = -56
    EX_FORBIDDEN = -55
    EX_LOOKUP_ERROR = -54
    EX_BAD_DATA = -53
    EX_SYNTAX_ERROR = -52
    EX_NODE_ERROR = -51
    EX_ASSERT_ERROR = -50

    # ThingsDB internal errors
    EX_TOO_LARGE_X = -6
    EX_REQUEST_TIMEOUT = -5
    EX_REQUEST_CANCEL = -4
    EX_WRITE_UV = -3
    EX_MEMORY = -2
    EX_INTERNAL = -1


_ERRMAP = {
    Err.EX_OPERATION_ERROR: OperationError,
    Err.EX_NUM_ARGUMENTS: NumArgumentsError,
    Err.EX_TYPE_ERROR: TypeError,
    Err.EX_VALUE_ERROR: ValueError,
    Err.EX_OVERFLOW: OverflowError,
    Err.EX_ZERO_DIV: ZeroDivisionError,
    Err.EX_MAX_QUOTA: MaxQuotaError,
    Err.EX_AUTH_ERROR: AuthError,
    Err.EX_FORBIDDEN: ForbiddenError,
    Err.EX_LOOKUP_ERROR: LookupError,
    Err.EX_BAD_DATA: BadDataError,
    Err.EX_SYNTAX_ERROR: SyntaxError,
    Err.EX_NODE_ERROR: NodeError,
    Err.EX_ASSERT_ERROR: AssertionError,
    Err.EX_TOO_LARGE_X: ResultTooLargeError,
    Err.EX_REQUEST_TIMEOUT: RequestTimeoutError,
    Err.EX_REQUEST_CANCEL: RequestCancelError,
    Err.EX_WRITE_UV: WriteUVError,
    Err.EX_MEMORY: MemoryError,
    Err.EX_INTERNAL: InternalError,
}

_PROTO_RESPONSE_MAP = {
    Proto.RES_PING: lambda f, d: f.set_result(None),
    Proto.RES_OK: lambda f, d: f.set_result(None),
    Proto.RES_DATA: lambda f, d: f.set_result(d),
    Proto.RES_ERROR: lambda f, d: f.set_exception(_ERRMAP.get(
        d['error_code'],
        ThingsDBError)(errdata=d)),
}

_PROTO_EVENTS = (
    Proto.ON_NODE_STATUS,
    Proto.ON_WATCH_INI,
    Proto.ON_WATCH_UPD,
    Proto.ON_WATCH_DEL,
    Proto.ON_WARN
)


def proto_unkown(f, d):
    f.set_exception(TypeError('unknown package type received ({})'.format(d)))


class Protocol(asyncio.Protocol):

    def __init__(
        self,
        on_connection_lost: Callable[[asyncio.Protocol, Exception], None],
        on_event: Callable[[Package], None],
        loop: Optional[asyncio.AbstractEventLoop] = None
    ):
        self._buffered_data = bytearray()
        self.package = None
        self.transport = None
        self.loop = asyncio.get_event_loop() if loop is None else loop
        self.close_future = None
        self._requests = {}
        self._pid = 0
        self._on_connection_lost = on_connection_lost
        self._on_event = on_event

    def connection_made(self, transport: asyncio.Transport) -> None:
        '''
        override asyncio.Protocol
        '''
        self.close_future = self.loop.create_future()
        self.transport = transport

    def connection_lost(self, exc: Exception) -> None:
        '''
        override asyncio.Protocol
        '''
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

        self.close_future.set_result(None)
        self.close_future = None
        self.transport = None
        self._on_connection_lost(self, exc)

    def data_received(self, data: bytes) -> None:
        '''
        override asyncio.Protocol
        '''
        self._buffered_data.extend(data)
        while self._buffered_data:
            size = len(self._buffered_data)
            if self.package is None:
                if size < Package.st_package.size:
                    return None
                self.package = Package(self._buffered_data)
            if size < self.package.total:
                return None
            try:
                self.package.extract_data_from(self._buffered_data)
            except KeyError as e:
                logging.error(f'Unsupported package received: {e}')
            except Exception as e:
                logging.exception(e)
                # empty the byte-array to recover from this error
                self._buffered_data.clear()
            else:
                tp = self.package.tp
                if tp in _PROTO_RESPONSE_MAP:
                    self._on_response(self.package)
                elif tp in _PROTO_EVENTS:
                    self._on_event(self.package)
                else:
                    logging.error(f'Unsupported package type received: {tp}')

            self.package = None

    def write(
            self,
            tp: Proto,
            data: Any = None,
            is_bin: bool = False,
            timeout: Optional[int] = None
    ) -> asyncio.Future:
        """Write data to ThingsDB.
        This will create a new PID and returns a Future which will be
        set when a response is received from ThingsDB, or time-out is reached.
        """
        if self.transport is None:
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

        self.transport.write(header + data)

        task = asyncio.ensure_future(
            self._timer(self._pid, timeout)) if timeout else None

        future = asyncio.Future()
        self._requests[self._pid] = (future, task)
        return future

    async def _timer(self, pid: int, timeout: Optional[int]) -> None:
        await asyncio.sleep(timeout)
        try:
            future, task = self._requests.pop(pid)
        except KeyError:
            logging.error('timed out package id not found: {}'.format(
                    self._data_package.pid))
            return None

        future.set_exception(TimeoutError(
            'request timed out on package id {}'.format(pid)))

    def _on_response(self, pkg: Package) -> None:
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

        _PROTO_RESPONSE_MAP.get(pkg.tp, proto_unkown)(future, pkg.data)
