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


ON_NODE_STATUS = 0
ON_WATCH_INI = 1
ON_WATCH_UPD = 2
ON_WATCH_DEL = 3
ON_WARN = 4

RES_PING = 16
RES_AUTH = 17
RES_QUERY = 18
RES_WATCH = 19
RES_UNWATCH = 20
RES_ERROR = 21

REQ_PING = 32
REQ_AUTH = 33
REQ_QUERY = 34
REQ_WATCH = 35
REQ_UNWATCH = 36
REQ_RUN = 37


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
    EX_OPERATION_ERROR: OperationError,
    EX_NUM_ARGUMENTS: NumArgumentsError,
    EX_TYPE_ERROR: TypeError,
    EX_VALUE_ERROR: ValueError,
    EX_OVERFLOW: OverflowError,
    EX_ZERO_DIV: ZeroDivisionError,
    EX_MAX_QUOTA: MaxQuotaError,
    EX_AUTH_ERROR: AuthError,
    EX_FORBIDDEN: ForbiddenError,
    EX_LOOKUP_ERROR: LookupError,
    EX_BAD_DATA: BadDataError,
    EX_SYNTAX_ERROR: SyntaxError,
    EX_NODE_ERROR: NodeError,
    EX_ASSERT_ERROR: AssertionError,
    EX_TOO_LARGE_X: ResultTooLargeError,
    EX_REQUEST_TIMEOUT: RequestTimeoutError,
    EX_REQUEST_CANCEL: RequestCancelError,
    EX_WRITE_UV: WriteUVError,
    EX_MEMORY: MemoryError,
    EX_INTERNAL: InternalError,
}


PROTOMAP = {
    RES_PING: lambda f, d: f.set_result(None),
    RES_AUTH: lambda f, d: f.set_result(None),
    RES_QUERY: lambda f, d: f.set_result(d),
    RES_WATCH: lambda f, d: f.set_result(None),
    RES_UNWATCH: lambda f, d: f.set_result(None),
    RES_ERROR:
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
        ON_NODE_STATUS,
        ON_WARN,
        ON_WATCH_INI,
        ON_WATCH_UPD,
        ON_WATCH_DEL,
    ), _responses=(
        RES_PING,
        RES_AUTH,
        RES_QUERY,
        RES_WATCH,
        RES_UNWATCH,
        RES_ERROR
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
