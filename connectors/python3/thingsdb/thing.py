class Thing:

    __slots__ = (
        '_client',
        '_collection',
        '_id',
        '_props'
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
        return await self._collection.client.query(
            query,
            target=self._collection.collection_id,
            **kwargs)

    async def assign(self, name, value, **kwargs):
        await self._query(f'thing({self._id}).{name}={value}', **kwargs)

    async def watch(self, name, value, **kwargs):
        await self._query(f'thing({self._id}).{name}={value}', **kwargs)

    # def __getattribute__(self, name):
    #     if name in self._props:
    #         return self._props[name]
    #     return None
