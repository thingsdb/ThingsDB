from .thing import Thing


class Collection(Thing):

    __slots__ = (
        '_id',
        '_props',
        '_attrs'
    )

    def __new__(cls, client, collection_id):
        thing = client._things.get(id)
        if thing is None:
            thing = object.__new__(cls)
            thing._client = client
            thing._id = collection_id
            thing = client._things[id] = super().__new__(
                cls,
                thing,
                collection_id)
        return thing