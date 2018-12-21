class Thing:

    __slots__ = (
        '_client',
        '_collection',
        '_id',
        '_props',
        '_attrs',
    )

    def _init(self, collection, id):
        self._collection = collection
        self._id = id
        self._props = {}
        self._attrs = {}

    def __new__(cls, collection, id):
        client = collection._client
        thing = client._things.get(id)
        if thing is None:
            thing = client._things[id] = super().__new__(cls)
            thing._init(collection, id)
        return thing

    async def _query(self, query, **kwargs):
        return await self._collection._client.query(
            query,
            target=self._collection._id,
            **kwargs)

    async def assign(self, name, value, **kwargs):
        await self._query(f'thing({self._id}).{name}={value}', **kwargs)

    def _assign(self, prop, value):
        if isinstance(value, dict):
            if '#' in value:
                thing_id = value['#']
                self._props[prop] = Thing(self._collection, thing_id)
        else:
            self._props[prop] = value

    async def watch(self, **kwargs):
        return await self._collection._client.watch(
            [self._id],
            collection=self._collection._id)

    def __getattr__(self, name):
        if name in self._props:
            return self._props[name]
        raise AttributeError(f'{self} has no property {name}')
