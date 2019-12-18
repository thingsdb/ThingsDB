import logging
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

    async def watch(self):
        collection = self._collection
        await collection._client.watch(self._id, scope=collection._scope)

    async def unwatch(self):
        collection = self._collection
        await collection._client.unwatch(self._id, scope=collection._scope)

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


class ThingStrict(Thing):

    __STRICT__ = True
