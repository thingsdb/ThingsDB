import asyncio
import logging
from .package import Package
from .exceptions import AuthError


REQ_PING = 0
REQ_AUTH = 1
REQ_EVENT = 2
REQ_GET_ELEM = 3


RES_ACK = 0
RES_RESULT = 1
RES_ERR_AUTH = 65
RES_ERR_NODE = 66
RES_ERR_TYPE = 67
RES_ERR_INDEX = 68
RES_ERR_RUNTIME = 69

TASK_USER_CREATE = 0
TASK_USER_DROP = 1
TASK_USER_ALTER = 2
TASK_DB_CREATE = 3
TASK_DB_RENAME = 4
TASK_DB_DROP = 5
TASK_GRANT = 6
TASK_REVOKE = 7
TASK_SET_REDUNDANCY = 8
TASK_NODE_ADD = 9
TASK_NODE_REPLACE = 10
TASK_SUBSCRIBE = 11
TASK_UNSUBSCRIBE = 12
TASK_PROPS_SET = 13
TASK_PROPS_DEL = 14


PROTOMAP = {
    RES_ACK: lambda f, d: f.set_result(None),
    RES_RESULT: lambda f, d: f.set_result(d),
    RES_ERR_AUTH: lambda f, d: f.set_exception(AuthError(d['error_msg']))
}


class Protocol(asyncio.Protocol):

    _connected = False

    def __init__(self):
        self._buffered_data = bytearray()
        self.package = None

    def connection_made(self, transport):
        '''
        override asyncio.Protocol
        '''

        logging.info('made')

    def connection_lost(self, exc):
        '''
        override asyncio.Protocol
        '''
        logging.info('lost')

    def data_received(self, data):
        '''
        override asyncio.Protocol
        '''
        self._buffered_data.extend(data)
        while self._buffered_data:
            size = len(self._buffered_data)
            if self.package is None:
                if size < Package.struct_datapackage.size:
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
