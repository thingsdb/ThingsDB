class Arr(tuple):

    def __init__(self, parent, *args):
        self._parent = parent
        super().__init__(*args)

    async def push(self, values):
        values = ','.join(repr(wrap(v)) for v in values)
        await self._parent._query(f'thing({self._parent._id}).push({values})')