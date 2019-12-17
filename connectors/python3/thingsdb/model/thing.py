import logging
from .keys import ARRAY_OF, SET_OF, REQUIRED, OPTIONAL
from .prop import Prop


def checkevent(f):
    def wrapper(self, event_id, *args):
        if self._event_id > event_id:
            logging.warning(
                f'ignore event because the current event `{self._event_id}` '
                f'is greather than the received event `{event_id}`')
            return
        self._event_id = event_id
        f(self, event_id, *args)
        self._collection.go_pending()
    return wrapper


class ThingHash:
    def __init__(self, id):
        self._id = id

    def __hash__(self):
        return self._id

    def __eq__(self, other):
        return self._id == other._id


class Thing(ThingHash):
    # When __STRICT__ is set to `True`, only properties which are defined in
    # the model class are assigned to a `Thing` instance. If `False`, all
    # properties are set, not only the ones defined by the model class.
    __STRICT__ = False

    # When __SET_ANYWAY__ is set to `True`, values which do mot match the
    # specification will be assigned to a `Thing` instance anyway and only
    # a warning will be logged. If `False`, the properties will not be set.
    __SET_ANYWAY__ = False

    _props = dict()
    _any = None
    _thing = None

    def __init__(self, collection, id: int):
        super().__init__(id)
        cls = self.__class__
        self._event_id = 0
        self._collection = collection
        cls._any = Prop.get_conv('any', klass=Thing, collection=collection)
        cls._thing = Prop.get_conv('thing', klass=Thing, collection=collection)
        collection.register(self)

    def __init_subclass__(cls):
        items = {
            k: v for k, v in cls.__dict__.items() if not k.startswith('__')}
        for key, val in items.items():
            if isinstance(val, str):
                val = val,
            if isinstance(val, tuple):
                prop = cls._props[key] = Prop(*val)
                delattr(cls, key)

    def __bool__(self):
        return bool(self._event_id)

    def __repr__(self):
        return f'#{self._id}'

    def id(self):
        return self._id

    @checkevent
    def on_init(self, event, data):
        self._job_set(data)

    @checkevent
    def on_update(self, event, jobs):
        for job_dict in jobs:
            for name, job in job_dict.items():
                jobfun = self._UPDMAP.get(name)
                if jobfun is None:
                    raise TypeError(f'unknown job `{name}` for `{self}`')
                jobfun(self, job)

    def on_delete(self):
        self._collection.pop(self)

    def _job_add(self, pair):
        cls = self.__class__
        (k, v), = pair.items()
        prop = cls._props.get(k)

        try:
            set_ = getattr(self, k)
        except AttributeError:
            if prop:
                logging.warning(
                    f'missing property `{k}` on `{self}` '
                    f'while the property is defined in the '
                    f'model class as `{prop.spec}`')
            return

        if not isinstance(set_, set):
            logging.warning(
                f'got a add job for property `{k}` on `{self}` '
                f'while the property is of type `{type(set_)}`')
            return

        convert = prop.nconv if prop else cls._thing
        try:
            set_.update((convert(item) for item in v))
        except Exception as e:
            logging.warning(
                f'got a value for property `{k}` on `{self}` which '
                f'does not match `{prop.spec if prop else "thing"}` ({e})')

    def _job_remove(self, pair):
        cls = self.__class__
        (k, v), = pair.items()

        try:
            set_ = getattr(self, k)
        except AttributeError:
            if prop:
                logging.warning(
                    f'missing property `{k}` on `{self}` '
                    f'while the property is defined in the '
                    f'model class as `{prop.spec}`')
            return

        if not isinstance(set_, set):
            logging.warning(
                f'got a remove job for property `{k}` on `{self}` '
                f'while the property is of type `{type(set_)}`')
            return

        set_.difference_update((ThingHash(id) for id in v))

    def _job_set(self, pairs):
        cls = self.__class__
        for k, v in pairs.items():
            prop = cls._props.get(k)
            if prop:
                convert = prop.vconv
            elif cls.__STRICT__:
                continue
            else:
                convert = cls._any(v)
            try:
                v = convert(v)
            except Exception as e:
                logging.warning(
                    f'got a value for property `{k}` on `{self}` which '
                    f'does not match `{prop.spec if prop else "any"}` ({e})')
                if not cls.__SET_ANYWAY__:
                    continue

            setattr(self, k, v)

        self._collection.go_pending()

    def _job_splice(self, pair):
        cls = self.__class__
        (k, v), = pair.items()
        prop = cls._props.get(k)

        try:
            arr = getattr(self, k)
        except AttributeError:
            if prop:
                logging.warning(
                    f'missing property `{k}` on `{self}` '
                    f'while the property is defined in the '
                    f'model class as `{prop.spec}`')
            return

        if not isinstance(arr, list):
            logging.warning(
                f'got a splice job for property `{k}` on `{self}` '
                f'while the property is of type `{type(arr)}`')
            return

        index, count, *items = v
        convert = prop.nconv if prop else cls._any
        try:
            arr[index:index+count] = (convert(item) for item in items)
        except (TypeError, ValueError) as e:
            logging.warning(
                f'got a value for property `{k}` on `{self}` '
                f'which does not match `{prop.spec if prop else "any"}` ({e})')

    def _job_del(self, k):
        prop = self.__class__._props.get(k)
        if prop:
            logging.warning(
                f'property `{k}` on `{self}` will be removed while it '
                f'is defined in the model class as `{prop.spec}`')
        try:
            delattr(self, k)
        except AttributeError:
            pass

    _UPDMAP = {
        'set': _job_set,
        'del': _job_del,
        'splice': _job_splice,
        'add': _job_add,
        'remove': _job_remove,
    }


class _Thing:

    __slots__ = (
        '_id',
        '_collection',
        '_event_id',
        '__weakref__',
    )

    __strict__ = True

    def id(self):
        return self._id

    def event_id(self):
        return self._event_id

    def as_dict(self):
        return self.__dict__

    def _init(self, id, collection):
        self._id = id
        self._collection = collection
        self._event_id = None
        collection._client._things[id] = self
        self._to_wqueue()

    def __new__(cls, id, collection):
        thing = collection._client._things.get(id)
        if thing is None:
            # when not subclassed, use _Thing so we get a clean __dict__ and
            # set __strict__ to False
            thing = super().__new__(_Thing if cls is Thing else cls)
            thing._init(id, collection)
        return thing

    def __init_subclass__(cls):
        required = dict()
        optional = list()

        for key, val in cls.__dict__.items():
            if isinstance(val, type):
                if hasattr(val, REQUIRED):
                    required[key] = val
                if hasattr(val, OPTIONAL):
                    optional.append(key)

        cls.__optional__ = tuple(optional)
        cls.__required__ = required

    def __bool__(self):
        return bool(self._event_id)

    def __repr__(self):
        return f't({self._id})'

    def __iter__(self):
        return self.__dict__.__iter__()

    def __getitem__(self, prop):
        return self.__dict__[prop]

    def _to_wqueue(self):
        self._collection._wqueue.add(self._id)

    @classmethod
    async def _build(cls, id, client, collection):
        for prop, clz in cls.__required__.items():
            alt = clz.__dict__[REQUIRED]
            if issubclass(alt, (list, tuple)):
                await client.query(
                    f'#{id}.{prop} = [];',
                    scope=collection.scope)
            elif issubclass(alt, Thing):
                nid = await client.query(
                    f'{collection.scope} #{id}.{prop} = {{}}).id();')
                await clz._build(nid, client, collection)
            elif issubclass(alt, set):
                await client.query(
                    f'{collection.scope} #{id}.{prop} = set();')
            else:
                fun = {
                    str: 'str',
                    float: 'float',
                    int: 'int',
                    bool: 'bool'
                }[alt]
                await client.query(
                    f'{collection.scope} #{id}.{prop} = {fun}();')

    def _check(self, attr, value):
        if value is None:
            return value, hasattr(attr, OPTIONAL)

        if issubclass(attr, Thing):
            if not isinstance(value, dict):
                return value, False

            thing_id = value.get('#')
            if thing_id is None:
                return value, False
            return attr(thing_id, self._collection), True

        is_valid = True

        if issubclass(attr, set):
            try:
                value = value['$']
            except (AttributeError, KeyError):
                return value, False
            except TypeError:
                value = value[1:]

            set_of = getattr(attr, SET_OF, None)
            if set_of is not None:
                for i, v in enumerate(value):
                    value[i], check = self._check(set_of, v)
                    is_valid = is_valid and check
                value = attr(value)

            return value, is_valid

        if not issubclass(attr, value.__class__):
            return value, False

        array_of = getattr(attr, ARRAY_OF, None)
        if array_of is not None:
            for i, v in enumerate(value):
                value[i], check = self._check(array_of, v)
                is_valid = is_valid and check
            value = attr(value)

        return value, is_valid

    def _do_event(self, event_id):
        if self._event_id > event_id:
            logging.warning(
                f'ignore event because the current event `{self._event_id}` '
                f'is greather than the received event `{event_id}`')
            return False
        self._event_id = event_id
        return True

    def _job_set(self, attrs):
        for prop, val in attrs.items():
            attr = getattr(self.__class__, prop, None)
            val, is_valid = (val, False) \
                if attr is None else self._check(attr, val)

            if not is_valid and self.__class__.__strict__:
                if attr is None:
                    logging.debug(
                        f'property `{prop}` on `{self}` is not defined by '
                        f'`{self.__class__.__name__}` and will be ignored')
                elif hasattr(attr, ARRAY_OF) \
                        and issubclass(attr, val.__class__):
                    logging.debug(
                        f'property `{prop}` on `{self}` is expecting an array '
                        f'of type `{attr._type.__name}` and will be ignored')
                else:
                    logging.debug(
                        f'property `{prop}` on `{self}` is expecting '
                        f'type `{attr.__name__}` and will be ignored')
                continue

            setattr(self, prop, val)

    def _job_del(self, prop):
        try:
            delattr(self, prop)
        except AttributeError:
            logging.debug(
                f'cannot delete property `{prop}` because it is '
                f'missing on `{self}`')

    def _job_splice(self, splice_job):
        for prop, value in splice_job.items():
            index, count, *items = value
            try:
                arr = getattr(self, prop)
            except AttributeError:
                logging.debug(
                    f'cannot use splice on property `{prop}` because '
                    f'the property is missing on `{self}`')
            else:
                items, is_valid = self._check(arr.__class__, items)
                if not is_valid and self.__class__.__strict__:
                    logging.critical(
                        f'splice on property `{prop}` on `{self}` got an '
                        f'item which is not of the specified type')
                arr[index:index+count] = items

    def _job_add(self, add_job):
        for prop, things in add_job.items():
            try:
                set_ = getattr(self, prop)
            except AttributeError:
                logging.debug(
                    f'cannot use add on property `{prop}` because '
                    f'the property is missing on `{self}`')
            else:
                things, is_valid = self._check(set_.__class__, things)

                if not is_valid and self.__class__.__strict__:
                    logging.critical(
                        f'job `add` on property `{prop}` on `{self}` got an '
                        f'item which is not of the specified type')
                set_.update(things)

    def _job_remove(self, remove_job):
        for prop, thing_ids in remove_job.items():
            try:
                set_ = getattr(self, prop)
            except AttributeError:
                logging.debug(
                    f'cannot use add on property `{prop}` because '
                    f'the property is missing on `{self}`')
            else:
                thing_ids = set(thing_ids[1:])
                set_.difference_update({t for t in set_ if t._id in thing_ids})

    async def on_init(self, event_id, data):
        """Called when an `init-watch` package is received.
        This method can be implemented be a custom thing but do not forget
        to make the `super().on_init(event_id, data)` call, unless you know
        what you are doing.
        """
        if not self._do_event(event_id):
            return
        self._job_set(data)
        for prop, cls in self.__required__.items():
            if prop not in self.__dict__:
                logging.critical(
                    f'property `{prop}` is not set on `{self}` but is marked '
                    f'as required. a default value will be applied')
                setattr(self, prop, cls())
        for prop in self.__optional__:
            if prop not in self.__dict__:
                setattr(self, prop, None)

        self._collection.go_wqueue()

    async def on_update(self, event_id, jobs):
        """Called when an `update-watch` package is received.
        This method can be implemented be a custom thing but do not forget
        to make the `super().on_update(event_id, jobs)` call, unless you know
        what you are doing.
        """
        if not self._do_event(event_id):
            return
        for job_dict in jobs:
            for name, job in job_dict.items():
                jobfun = self._UPDMAP.get(name)
                if jobfun is None:
                    raise TypeError(f'unknown job `{name}` for `{self}`')
                jobfun(self, job)
        self._collection.go_wqueue()

    async def on_delete(self):
        """Called when an `delete-watch` package is received.
        This method can be implemented be a custom thing but do not forget
        to make the `super().on_delete()` call, unless you know
        what you are doing.
        """
        self._collection._client._things.pop(self._id)

    _UPDMAP = {
        'set': _job_set,
        'del': _job_del,
        'splice': _job_splice,
        'add': _job_add,
        'remove': _job_remove,
    }


class ThingStrict(Thing):

    __STRICT__ = True
