class Thing:

    def __init__(self, collection, id):
        self._collection = collection
        self._id = id

    async def _query(self, query, **kwargs):
        return await self._collection.client.query(
            query,
            target=self._collection.collection_id,
            **kwargs)

    async def assign(self, name, value, **kwargs):
        await self._query(f'thing({self._id}).{name}={value}', **kwargs)

    async def watch(self, name, value, **kwargs):
        await self._query(f'thing({self._id}).{name}={value}', **kwargs)

