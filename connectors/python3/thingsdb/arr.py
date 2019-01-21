import asyncio
from .wrap import wrap


class Arr(list):

    __slots__ = (
        '_parent',
        '_prop',
        '_watchclass',
    )

    def __init__(self, parent, prop, *args):
        self._parent = parent
        self._prop = prop
        self._watchclass = None
        super().__init__(*args)

    def set_watcher(self, watchclass):
        self._watchclass = watchclass
        self.apply_watch()

    def watchclass(self):
        return self._watchclass

    def apply_watch(self, sl=slice(None)):
        if self._watchclass is None:
            return
        client = self._parent._collection._client
        asyncio.ensure_future(
            client.watch((
                t.set_watcher(self._watchclass)
                for t in self[sl]
            )),
            loop=client._loop
        )

    async def fetch(self):
        thing = self._parent
        res = await self._parent._query(
            f'thing({self._parent._id}).{self._prop}.map(_=>_)'
        )
        self[:] = [thing._unpack(None, v) for v in res]
        for t in self:
            t._is_fetched = True

    async def ti_push(self, value, **kwargs):
        if self._prop is None:
            raise TypeError('cannot run push on tuple type')
        blobs = kwargs.pop('blobs', [])
        value = wrap(value, blobs)
        if blobs:
            kwargs['blobs'] = blobs

        await self._parent._query(
            f'thing({self._parent._id}).{self._prop}.push({value})',
            **kwargs
        )

    async def ti_extend(self, values, **kwargs):
        if self._prop is None:
            raise TypeError('cannot run push on tuple type')
        blobs = kwargs.pop('blobs', [])
        value = ','.join(repr(wrap(v, blobs)) for v in values)
        if blobs:
            kwargs['blobs'] = blobs

        await self._parent._query(
            f'thing({self._parent._id}).{self._prop}.push({values})',
            **kwargs
        )