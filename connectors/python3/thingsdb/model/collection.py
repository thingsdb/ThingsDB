import asyncio
import weakref
from typing import Iterable, Optional, Union, TextIO, Type, Any
from ..client import Client
from .eventhandler import EventHandler
from .thing import Thing


class Collection(Thing):

    __STRICT__ = True
    __AS_TYPE__ = False

    def __init__(self):
        self._things = weakref.WeakValueDictionary()
        self._name = \
            getattr(self, '__COLLECTION_NAME__', self.__class__.__name__)
        self._scope = f'//{self._name}'
        self._pending = set()  # Thing ID's
        self._client = None  # use load, build or rebuild
        self._id = None
        self._types = {}  # mapping where the keys is a type_id
        for p in self._props.values():
            p.unpack(self)

    async def load(self, client: Client, load_procedures: bool = True) -> None:
        assert self._client is None, 'This collection is already loaded'
        self._client = client
        id = await self._client.query('.id()', scope=self._scope)
        super().__init__(self, id)
        client.add_event_handler(EventHandler(self))
        await self._client.watch(id, scope=self._scope)
        await self.refresh_types()
        if load_procedures:
            await self.load_procedures()

    async def load_procedures(self) -> None:
        """Load (re-load) all procedures.

        Procedures in the collection will be added to the collection as async
        methods.
        """
        assert self._client is not None, 'This collection is not loaded'

        procedures = await self.query('procedures_info();')
        for procedure in procedures:
            print(procedure)

    async def refresh_types(self) -> None:
        """Refresh types info.

        Procedures in the collection will be added to the collection as async
        methods.
        """
        assert self._client is not None, 'This collection is not loaded'

        self._types = {}
        types_info = await self.query('types_info();')
        for type_info in types_info:
            self._update_type(type_info)

    def _update_type(self, data):
        self._types[data['type_id']] = tuple(k[0] for k in data['fields'])

    async def build(
            self,
            client: Client,
            classes: Optional[Iterable[Type[Thing]]] = None,
            scripts: Optional[Iterable[Union[TextIO, str]]] = None,
            delete_if_exists: bool = False,
    ) -> None:
        """Build the collection in ThingsDB.

        This will create the collection in ThingsDB will default values for
        all collection properties.

        Args:
            client (Client):
                ThingsDB Client instance with an active, authenticated
                connection.
            classes (iterable, optional):
                Optional list of classes to create. This is only required when
                a class has no relation with the collection, otherwise the
                class will be created recursively while building the
                collection. Defaults to `None`.
            scripts (iterable, optional):
                Optional list of script which will be started after building
                the collection. They will be started in the same order as they
                are given. The iterable may contain File Objects in Text mode,
                or plain strings with ThingsDB code. Defaults to `None`.
            delete_if_exists (bool):
                When `True`, the collection will be removed if it exists. Be
                careful since all data in the collection will be removed! If
                this arguments is `False`, a `KeyError` will be raised when the
                collection exists. Defaults to `False`.
        """
        assert self._client is None, 'This collection is already loaded'
        if (await client.has_collection(self._name)):
            if delete_if_exists:
                await client.del_collection(self._name)
            else:
                raise KeyError(f'Collection `{self._name}` already exists')

        # create the collection, we are sure it does not exists
        await client.new_collection(self._name)

        # first create the types so circular dependencies may be handled
        await self._new_type(client, self)
        if classes:
            for model in classes:
                await model._new_type(client, self)

        # set the type definitions and
        await self._set_type(client, self)
        if classes:
            for model in classes:
                await model._set_type(client, self)

        for script in scripts:
            code = script if isinstance(script, str) else script.read()
            await client.query(
                code=code,
                scope=self._scope,
                convert_vars=False)

    async def query(self, code: str, **kwargs: Any) -> Any:
        """Query using this collection as scope.

        This is the same as calling the `query(..)` method on the client with
        the scope='...' argument set to the collection scope. All keyword
        arguments will be parsed to the `Client().query(..)` method so look at
        that method for more information.
        """
        return await self._client.query(code, scope=self._scope, **kwargs)

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
