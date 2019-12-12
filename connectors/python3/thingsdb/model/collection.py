import asyncio
from ..client.protocol import Proto
from .thing import Thing


class Collection(Thing):

    __slots__ = (
        '_client',
        '_scope',
        '_proto',
        '_wqueue',
    )

    def __init__(self, client, build=False, rebuild=False):
        self._scope = f'@:{self.__class__.__name__}'
        self._client = client
        self._wqueue = set()
        self._event_id = 0
        asyncio.ensure_future(
            self._async_init(build, rebuild),
            loop=client._loop)

    def __new__(cls, *args, **kwargs):
        # make sure Thing.__new__ will not be called
        return cls

    @property
    def scope(self):
        return self._scope

    async def _async_init(self, build=False, rebuild=False):
        if rebuild:
            try:
                await self._client.del_collection(self)
            except IndexError:
                pass
            build = rebuild
        try:
            collection_id = await self._client.query('.id()', target=self)
        except IndexError as e:
            if not build:
                raise e
            name = scope_get_name(self)
            collection_id = await self._client.new_collection(name)
            print('\n\n', collection_id)
            await self._build(collection_id, self._client, self)
            if asyncio.iscoroutinefunction(build):
                await build(self._client, self)

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
