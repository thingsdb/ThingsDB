import struct
import asyncio
import logging
import qpack


PROTO_PING = 0
PROTO_AUTH = 1

PROTO_ACK = 0
PROTO_ERR = 64
PROTO_ERR_AUTH = 65


class Rql:
    def __init__(self, loop=None):
        self._loop = loop if loop else asyncio.get_event_loop()
        self._username = None
        self._password = None
        self._host = None
        self._port = None
        self._transport = None
        self._protocol = None
        self._requests = {}

    async def connect(self, host, port=9200, timeout=5):
        self._host = host
        self._port = port
        conn = loop.create_connection(
            _RqlProtocol,
            host=self._host,
            port=self._port)
        self._transport, self._protocol = await asyncio.wait_for(
            conn, 
            timeout=timeout)
        self._pid = 0

    async def authenticate(self, username, password, timeout=5):
        self._username = username
        self._password = password
        future = self.write_package(
            PROTO_AUTH, 
            data=(self._username, self._password),
            timeout=timeout)
        resp = await future
        return resp

    async def _timeout_request(self, pid, timeout):
        await asyncio.sleep(timeout)
        if not self._requests[pid][0].cancelled():
            self._requests[pid][0].set_exception(TimeoutError(
                'request timed out on package id {} ({})'
                .format(pid)))
        del self._requests[pid]

    def write_package(self, tp, data=None, is_bin=False, timeout=3600):
        self._pid += 1
        self._pid %= 65536  # pid is handled as uint16_t

        data = data if is_bin else b'' if data is None else qpack.packb(data)

        header = DataPackage.struct_datapackage.pack(
            len(data),
            self._pid,
            tp,
            tp ^ 255)

        self._transport.write(header + data)

        task = asyncio.ensure_future(self._timeout_request(self._pid, timeout))
        future = asyncio.Future()
        self._requests[self._pid] = (future, task)
        return future


class DataPackage(object):

    __slots__ = ('pid', 'length', 'tipe', 'checkbit', 'data')

    struct_datapackage = struct.Struct('<IHBB')

    def __init__(self, barray):
        self.length, self.pid, self.tipe, self.checkbit = \
            self.__class__.struct_datapackage.unpack_from(barray, offset=0)
        self.length += self.__class__.struct_datapackage.size
        self.data = None

    def extract_data_from(self, barray):
        try:
            self.data = self.__class__._MAP[protomap.MAP_RES_DTYPE[self.tipe]](
                barray[self.__class__.struct_datapackage.size:self.length])
        finally:
            del barray[:self.length]



class _RqlProtocol(asyncio.Protocol):

    _connected = False



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
        logging.info('received')


async def test():
    rql = Rql()
    await rql.connect('localhost')
    await rql.authenticate('iris', 'siri')

    
if __name__ == '__main__':
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test())
