import asyncio
import weakref
from ..client import Client
from .eventhandler import EventHandler
from .thing import Thing


class Collection(Thing):

    def __init__(self):
        self._things = weakref.WeakValueDictionary()
        self._name = \
            getattr(self, '__NAME__', self.__class__.__name__)
        self._scope = f'//{self._name}'
        self._pending = set()  # Thing ID's
        self._client = None  # use load, build or rebuild
        self._id = None
        for p in self._props.values():
            p.unpack(self)

    async def load(self, client: Client) -> None:
        assert self._client is None, 'This collection is already loaded'
        self._client = client
        id = await self._client.query('.id()', scope=self._scope)
        super().__init__(self, id)
        client.add_event_handler(EventHandler(self))
        await self._client.watch(id, scope=self._scope)

    def on_reconnect(self):
        """Called from the `EventHandler`."""
        self._pending.update(self._things.keys())

        self.go_pending()

    def add_pending(self, thing):
        self._pending.add(thing.id())

    def pop(self, thing):
        self._things.pop(thing.id())

    def go_pending(self):
        if not self._pending:
            return
        future = asyncio.ensure_future(
            self._client.watch(*self._pending, scope=self._scope),
            loop=self._client._loop
        )
        self._pending.clear()
        return future

    def register(self, thing: Thing) -> None:
        self._things[thing._id] = thing


# class _Collection(Thing):

#     __slots__ = (
#         '_client',
#         '_scope',
#         '_proto',
#         '_wqueue',
#     )

#     def __init__(self, client):
#         self._scope = f'@:{self.__class__.__name__}'
#         self._client = client
#         self._watch_queue = set()
#         self._event_id = 0

#     def __new__(cls, *args, **kwargs):
#         # make sure Thing.__new__ will not be called
#         return cls

#     @property
#     def scope(self):
#         return self._scope

#     async def _async_init(self, build=False, rebuild=False):
#         if rebuild:
#             try:
#                 await self._client.del_collection(self)
#             except IndexError:
#                 pass
#             build = rebuild
#         try:
#             collection_id = await self._client.query('.id()', target=self)
#         except IndexError as e:
#             if not build:
#                 raise e
#             name = scope_get_name(self)
#             collection_id = await self._client.new_collection(name)
#             print('\n\n', collection_id)
#             await self._build(collection_id, self._client, self)
#             if asyncio.iscoroutinefunction(build):
#                 await build(self._client, self)

#         Thing._init(self, collection_id, self)
#         self._collection.go_wqueue()

#     def go_wqueue(self):
#         if not self._wqueue:
#             return

#         data = {
#             'things': list(self._wqueue),
#             'collection': self._scope
#         }

#         future = asyncio.ensure_future(
#             self._client._write_package(REQ_WATCH, data, timeout=10),
#             loop=self._client._loop
#         )

#         self._wqueue.clear()
#         return future


class CollectionStrict(Collection):

    __STRICT__ = True
