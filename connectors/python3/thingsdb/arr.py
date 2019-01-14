class Arr(list):

    def __init__(self, parent, *args):
        self._parent = parent
        super().__init__(*args)

    async def push(self, values, **kwargs):
        blobs = kwargs.pop('blobs', [])
        values = ','.join(repr(wrap(v, blobs)) for v in values)
        if blobs:
            kwargs['blobs'] = blobs

        await self._parent._query(
            f'thing({self._parent._id}).push({values})',
            **kwargs)
