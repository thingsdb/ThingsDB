import asyncio
from ..client.scope import Scope, thingsdb
from ..client.protocol import REQ_WATCH
from .thing import Thing


class Collection(Scope, Thing):

    __slots__ = (
        '_client',
        '_scope',
        '_wqueue',
    )

    def __init__(self, client, build=False, rebuild=False):
        Scope.__init__(self, self.__class__.__name__)
        self._client = client
        self._wqueue = set()
        self._event_id = 0
        asyncio.ensure_future(self._async_init(), loop=client._loop)

    def __new__(cls, *args, **kwargs):
        # make sure Thing.__new__ will not be called
        return Scope.__new__(cls)

    async def _async_init(self, build=False, rebuild=False):
        if rebuild:
            self._client.del_collection(self)
        collection_id = await self._client.query('id()', target=self)
        Thing._init(self, collection_id, self)
        self._collection.go_wqueue()

    def go_wqueue(self):
        if not self._wqueue:
            return

        data = {
            'things': list(self._wqueue),
            'collection': self._scope
        }

        future = asyncio.ensure_future(
            self._client._write_package(REQ_WATCH, data, timeout=10),
            loop=self._client._loop
        )

        self._wqueue.clear()
        return future
