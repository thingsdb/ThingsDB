from .arrayof import ArrayOf


class Thing:

    __strict__ = True

    __slots__ = (
        '_id',
        '_collection',
        '_event_id',
        '__weakref__',
    )

    def _init(self, id, collection):
        self._id = id
        self._collection = collection
        self._event_id = 0
        collection._client._things[id] = self
        self._to_wqueue()

    def __new__(cls, id, collection):
        if id is None:
            # called when reating a collection, `_init` will be called later
            return super().__new__(cls)
        thing = collection._client._things.get(id)
        if thing is None:
            thing = super().__new__(cls)
            thing._init(id, collection)
        return thing

    def __bool__(self):
        return bool(self._event_id)

    def __repr__(self):
        return f't({self._id})'

    def _to_wqueue(self):
        self._collection._wqueue.add(self._id)

    def _check(self, attr, value):
        if issubclass(attr, Thing):
            if not isinstance(value, dict):
                return value, False

            thing_id = value.get('#')
            if thing_id is None:
                return value, False
            thing = attr(thing_id, self._collection)
            return thing, True
        elif not issubclass(attr, value.__class__):
            return value, False

        if issubclass(attr, ArrayOf):
            arr = attr()
            is_valid = True
            for v in value:
                v, check = self._check(attr.type_, v)
                arr.append(v)
                if not check:
                    is_valid = False
            return arr, is_valid

        return value, True

    def _do_event(self, event_id):
        if self._event_id > event_id:
            logging.warning(
                f'ignore event because the current event `{self._event_id}` '
                f'is greather than the received event `{event_id}`')
            return False
        self._event_id = event_id
        return True

    def _assign(self, prop, val):
        attr = getattr(self.__class__, prop, None)
        val, is_valid = (val, False) \
            if attr is None else self._check(attr, val)

        if not is_valid and self.__class__.__strict__:
            if attr is None:
                logging.debug(
                    f'property `{prop}` is not defined and will be ignored')
            else:
                logging.debug(
                    f'property `{prop}` is expecting '
                    f'type `{attr.__name__}`')
            return

        setattr(self, prop, val)

    def _job_assign(self, attrs):
        for prop, value in attrs.items():
            self._assign(prop, value)

    def _job_del(self, prop):
        try:
            delattr(self, prop)
        except AttributeError:
            logging.debug(
                f'cannot delete property `{prop}` because it is missing')

    def _job_rename(self, props):
        for old_prop, new_prop in props.items():
            try:
                value = getattr(self, old_prop)
            except AttributeError:
                logging.debug(
                    f'cannot rename property `{prop}` because it is missing')
            else:
                setattr(self, new_prop, value)

    def _job_splice(self, job):
        for prop, value in job.items():
            index, count, new, *items = value
            try:
                arr = getattr(self, prop)
            except AttributeError:
                logging.debug(
                    f'cannot use splice on property `{prop}` because '
                    f'the property is missing')
            else:
                items, is_valid = self._check(arr.__class__, items)
                if not is_valid and self.__class__.__strict__:
                    logging.error(
                        f'splice on property `{prop}` has failed '
                        f'the checks')
                arr[index:index+count] = items

    async def on_watch_init(self, event_id, thing_dict):
        if not self._do_event(event_id):
            return
        for prop, value in thing_dict.items():
            self._assign(prop, value)
        self._collection.go_wqueue()

    async def on_watch_update(self, event_id, jobs):
        if not self._do_event(event_id):
            return
        for job_dict in jobs:
            for name, job in job_dict.items():
                jobfun = self._UPDMAP.get(name)
                if jobfun is None:
                    raise TypeError(f'unknown job `{name}` for thing {self}')
                jobfun(self, job)
        self._collection.go_wqueue()

    async def on_watch_delete(self):
        self._collection._client._things.pop(self._id)

    _UPDMAP = {
        'assign': _job_assign,
        'del': _job_del,
        'rename': _job_rename,
        'splice': _job_splice,
    }