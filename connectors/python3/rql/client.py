import asyncio
import struct
import logging
import qpack
from .package import Package
from .protocol import Protocol
from .protocol import REQ_AUTH
from .protocol import REQ_EVENT
from .protocol import REQ_GET_ELEM
from .protocol import PROTOMAP
from .protocol import proto_unkown
from .db import Db


class Client:
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
        conn = self._loop.create_connection(
            Protocol,
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

        PROTOMAP.get(pkg.tp, proto_unkown)(future, pkg.data)

    async def authenticate(self, username, password, timeout=5):
        self._username = username
        self._password = password
        future = self.write_package(
            REQ_AUTH,
            data=(self._username, self._password),
            timeout=timeout)
        await future

    async def get_database(self, target):
        root = await self._req_elem({target: -1})
        db = Db(self, target, root)
        return db

    async def _req_event(self, event, timeout=5):
        future = self.write_package(
            REQ_EVENT,
            data=event,
            timeout=timeout)
        resp = await future
        return resp

    async def _req_elem(self, req, timeout=5):
        future = self.write_package(
            REQ_GET_ELEM,
            data=req,
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

        header = Package.struct_datapackage.pack(
            len(data),
            self._pid,
            tp,
            tp ^ 255)

        self._transport.write(header + data)

        task = asyncio.ensure_future(self._timeout_request(self._pid, timeout))
        future = asyncio.Future()
        self._requests[self._pid] = (future, task)
        return future


