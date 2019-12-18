import asyncio
import weakref
from ..client import Client
from .eventhandler import EventHandler
from .thing import Thing


class Collection(Thing):

    __STRICT__ = True

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
