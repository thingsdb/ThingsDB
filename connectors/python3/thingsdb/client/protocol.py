import enum
import asyncio
import logging
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
    ON_WARN = 4

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


ERRMAP = {
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


PROTOMAP = {
    Proto.RES_PING: lambda f, d: f.set_result(None),
    Proto.RES_OK: lambda f, d: f.set_result(None),
    Proto.RES_DATA: lambda f, d: f.set_result(d),
    Proto.RES_ERROR:
        lambda f, d: f.set_exception(ERRMAP.get(
            d['error_code'],
            ThingsDBError)(errdata=d)),
}


def proto_unkown(f, d):
    f.set_exception(TypeError('unknown package type received ({})'.format(d)))


class Protocol(asyncio.Protocol):

    def __init__(self, on_lost, loop=None):
        self._buffered_data = bytearray()
        self.package = None
        self.transport = None
        self.loop = asyncio.get_event_loop() if loop is None else loop
        self.close_future = None
        self._on_lost = on_lost

    def connection_made(self, transport):
        '''
        override asyncio.Protocol
        '''
        self.close_future = self.loop.create_future()
        self.transport = transport

    def connection_lost(self, exc):
        '''
        override asyncio.Protocol
        '''
        self.close_future.set_result(None)
        self.close_future = None
        self.transport = None
        self._on_lost(self, exc)

    def data_received(self, data, _events=(
        Proto.ON_NODE_STATUS,
        Proto.ON_WARN,
        Proto.ON_WATCH_INI,
        Proto.ON_WATCH_UPD,
        Proto.ON_WATCH_DEL,
    ), _responses=(
        Proto.RES_PING,
        Proto.RES_OK,
        Proto.RES_DATA,
        Proto.RES_ERROR
    )):
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
                if tp in _events:
                    self.on_event_received(self.package)
                elif tp in _responses:
                    self.on_response_received(self.package)
                else:
                    logging.error(f'Unsupported package type received: {tp}')

            self.package = None

    @staticmethod
    def on_event_received(pkg):
        raise NotImplementedError

    @staticmethod
    def on_package_received(pkg):
        raise NotImplementedError
