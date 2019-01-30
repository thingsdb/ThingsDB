import asyncio
import logging
from .package import Package
from .exceptions import OverflowError
from .exceptions import ZeroDivisionError
from .exceptions import MaxQuotaError
from .exceptions import AuthError
from .exceptions import ForbiddenError
from .exceptions import IndexError
from .exceptions import BadRequestError
from .exceptions import QueryError
from .exceptions import NodeError
from .exceptions import InternalError


REQ_PING = 32
REQ_AUTH = 33
REQ_QUERY = 34

REQ_WATCH = 48
REQ_UNWATCH = 49

RES_PING = 64
RES_AUTH = 65
RES_QUERY = 66

RES_WATCH = 80
RES_UNWATCH = 81

ON_WATCH_INI = 16
ON_WATCH_UPD = 17
ON_WATCH_DEL = 18
ON_NODE_STATUS = 19

RES_ERR_OVERFLOW = 96
RES_ERR_ZERO_DIV = 97
RES_ERR_MAX_QUOTA = 98
RES_ERR_AUTH = 99
RES_ERR_FORBIDDEN = 100
RES_ERR_INDEX = 101
RES_ERR_BAD_REQUEST = 102
RES_ERR_QUERY = 103
RES_ERR_NODE = 104
RES_ERR_INTERNAL = 127

ON_WATCH = (ON_WATCH_INI, ON_WATCH_UPD, ON_WATCH_DEL, ON_NODE_STATUS)

PROTOMAP = {
    RES_PING: lambda f, d: f.set_result(None),
    RES_AUTH: lambda f, d: f.set_result(None),
    RES_QUERY: lambda f, d: f.set_result(d),
    RES_WATCH: lambda f, d: f.set_result(None),
    RES_UNWATCH: lambda f, d: f.set_result(None),
    RES_ERR_OVERFLOW:
        lambda f, d: f.set_exception(OverflowError(errdata=d)),
    RES_ERR_ZERO_DIV:
        lambda f, d: f.set_exception(ZeroDivisionError(errdata=d)),
    RES_ERR_MAX_QUOTA:
        lambda f, d: f.set_exception(MaxQuotaError(errdata=d)),
    RES_ERR_AUTH:
        lambda f, d: f.set_exception(AuthError(errdata=d)),
    RES_ERR_FORBIDDEN:
        lambda f, d: f.set_exception(ForbiddenError(errdata=d)),
    RES_ERR_INDEX:
        lambda f, d: f.set_exception(IndexError(errdata=d)),
    RES_ERR_BAD_REQUEST:
        lambda f, d: f.set_exception(BadRequestError(errdata=d)),
    RES_ERR_QUERY:
        lambda f, d: f.set_exception(QueryError(errdata=d)),
    RES_ERR_NODE:
        lambda f, d: f.set_exception(NodeError(errdata=d)),
    RES_ERR_INTERNAL:
        lambda f, d: f.set_exception(InternalError(errdata=d)),
}


def proto_unkown(f, d):
    f.set_exception(TypeError('unknown package type received ({})'.format(d)))


class Protocol(asyncio.Protocol):

    def __init__(self, loop=None):
        self._buffered_data = bytearray()
        self.package = None
        self.transport = None
        self.loop = asyncio.get_event_loop() if loop is None else loop
        self.close_future = None

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

    def data_received(self, data):
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
                logging.error('Unsupported package received: {}'.format(e))
            except Exception as e:
                logging.exception(e)
                # empty the byte-array to recover from this error
                self._buffered_data.clear()
            else:
                self.on_package_received(self.package)
            self.package = None

    @staticmethod
    def on_package_received(pkg):
        raise NotImplementedError
