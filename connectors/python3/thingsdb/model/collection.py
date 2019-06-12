import asyncio
from ..client.scope import Scope
from ..client.protocol import REQ_WATCH
from .thing import Thing


class Collection(Scope, Thing):

    __slots__ = (
        '_client',
        '_scope',
        '_wqueue',
    )

    def __init__(self, client):
        Scope.__init__(self, self.__class__.__name__)
        self._client = client
        self._wqueue = set()
        self._event_id = 0
        asyncio.ensure_future(self._async_init(), loop=client._loop)

    def __new__(cls, client):
        return super().__new__(cls, None, None)

    async def _async_init(self):
        try:
            collection_id = await self._client.query('id()', target=self)
        except IndexError:
            collection_id = await self._client.query(
                f'new_collection("{self.__class__.__name__}");',
                target=self._client.thingsdb)
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
