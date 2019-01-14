from .onwatch import OnWatch
from .repesent import repesent


class T(dict):

    def _comma_sep(self):

        return ','.join(f'{k}:{v}' for k, v in)

    def __repr__(self):
        for k, v in self:
        return '{{}}'


class Thing:

    __slots__ = (
        '_client',
        '_collection',
        '_id',
        '_props',
        '_attrs',
        '_on_watch',
    )

    def _init(self, collection, id):
        self._collection = collection
        self._id = id
        self._props = {}
        self._attrs = None
        self._on_watch = OnWatch

    def __new__(cls, collection, id):
        client = collection._client
        thing = client._things.get(id)
        if thing is None:
            thing = client._things[id] = super().__new__(cls)
            thing._init(collection, id)
        return thing

    def __repr__(self):
        return f'{{#:{self._id}}}'

    async def _query(self, query, **kwargs):
        return await self._collection._client.query(
            query,
            target=self._collection._id,
            **kwargs)

    def _unpack(self, value, _arr=(list, tuple)):
        if isinstance(value, dict):
            if '#' in value:
                thing_id = value['#']
                return Thing(self._collection, thing_id)
        elif isinstance(value, _arr):
            return (self._unpack(v) for v in value)

    def _assign(self, prop, value):
        self._props[prop] = self._unpack(value)

    def __getattr__(self, name):
        if name in self._props:
            return self._props[name]
        raise AttributeError(f'{self} has no property {name}')

    def _job_assign(self, job):
        for prop, value in job.items():
            self._assign(prop, value)

    async def assign(self, name, value, **kwargs):
        blobs = kwargs.pop('blobs', [])
        value = wrap(value, blobs)
        if blobs:
            kwargs['blobs'] = blobs

        await self._query(f'thing({self._id}).{name}={value!r}', **kwargs)

    async def fetch(self):
        thing_dict = await self._query(f'thing({self._id})')
        for prop, value in thing_dict.items():
            self._assign(prop, value)

    async def watch(self, **kwargs):
        return await self._collection._client.watch(
            [self._id],
            collection=self._collection._id)

    def set_watcher(self, watchclass):
        assert watchclass.on_init
        assert watchclass.on_update
        assert watchclass.on_delete
        self._on_watch = watchclass
        return self

    def on_watch_init(self, thing_dict):
        for prop, value in thing_dict.items():
            self._assign(prop, value)

    def on_watch_update(self, jobs):
        for job_dict in jobs:
            for name, job in job_dict.items():
                if name == 'assign':
                    self._job_assign(job)

    def on_watch_delete(self):
        self._collection._client._things.pop(self._id)
