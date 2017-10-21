import struct
import asyncio
import logging
import qpack


REQ_PING = 0
REQ_AUTH = 1
REQ_EVENT = 2


RES_ACK = 0
RES_RESULT = 1
RES_ERR_AUTH = 65
RES_ERR_NODE = 66
RES_ERR_TYPE = 67
RES_ERR_INDEX = 68
RES_ERR_RUNTIME = 69


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
        self._protocol.on_package_received = self._on_package_received
        self._pid = 0

    def _on_package_received(self, pkg):
        logging.debug(pkg)
        try:
            future, task = self._requests.pop(pkg.pid)
        except KeyError:
            logging.error('package ID not found: {}'.format(
                    self._data_package.pid))
            return None

        # cancel the timeout task
        task.cancel()

        if future.cancelled():
            return

        future.set_result(pkg.data)

    async def authenticate(self, username, password, timeout=5):
        self._username = username
        self._password = password
        future = self.write_package(
            REQ_AUTH,
            data=(self._username, self._password),
            timeout=timeout)
        resp = await future
        return resp

    async def trigger(self, event, timeout=5):
        future = self.write_package(
            REQ_EVENT,
            data=event,
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

    __slots__ = ('pid', 'length', 'total', 'tipe', 'checkbit', 'data')

    struct_datapackage = struct.Struct('<IHBB')

    def __init__(self, barray):
        self.length, self.pid, self.tipe, self.checkbit = \
            self.__class__.struct_datapackage.unpack_from(barray, offset=0)
        self.total = self.__class__.struct_datapackage.size + self.length
        self.data = None

    def extract_data_from(self, barray):
        try:
            self.data = qpack.unpackb(
                barray[self.__class__.struct_datapackage.size:self.total]) \
                if self.length else None
        finally:
            del barray[:self.total]

    def __repr__(self):
        return '<id: {0.pid} size: {0.length} tp: {0.tipe}>'.format(self)


class _RqlProtocol(asyncio.Protocol):

    _connected = False

    def __init__(self):
        self._buffered_data = bytearray()
        self._data_package = None

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
            if self._data_package is None:
                if size < DataPackage.struct_datapackage.size:
                    return None
                self._data_package = DataPackage(self._buffered_data)
            if size < self._data_package.total:
                return None
            try:
                self._data_package.extract_data_from(self._buffered_data)
            except KeyError as e:
                logging.error('Unsupported package received: {}'.format(e))
            except Exception as e:
                logging.exception(e)
                # empty the byte-array to recover from this error
                self._buffered_data.clear()
            else:
                self.on_package_received(self._data_package)
            self._data_package = None

    @staticmethod
    def on_package_received(pkg):
        raise NotImplementedError


async def test():
    rql = Rql()
    await rql.connect('localhost')
    res = await rql.authenticate('iris', 'siri')
    print(res)
    res = await rql.trigger({
        '_': [{
            '_t': 0,
            '_u': 'iriss',
            '_p': 'siri'
        }]
    })
    print(res)


if __name__ == '__main__':
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test())
