from .event import Event
from .protocol import TASK_PROPS_SET


def thing_set_props(thing, dmap):
    for prop, val in dmap.items():
        assert (prop[0] != '_'), 'unexpected prop: {}'.format(prop)
        if isinstance(val, dict):
            val = Elem(thing._db, val)
        if isinstance(val, (list, tuple)):
            val = [
                Elem(thing._db, v)
                if isinstance(v, dict) else v for v in val]
        setattr(thing, prop, val)


def thing_map_props(event, dmap, props):
    for prop, val in props.items():
        if isinstance(val, Elem):
            val = {'_i': val._id}
        elif isinstance(val, (list, tuple)):
            val = [{'_i': v._id} if isinstance(v, Elem) else v for v in val]
        dmap[prop] = val


class ElemMeta(type):
    def __call__(self, *args):
        if len(args) == 3:
            return super().__call__(*args)

        db, dmap = args
        id = dmap.pop('_i')
        thing = db._things.get(id, None)
        if thing is None:
            thing = db._things[id] = super().__call__(db, id)

        thing_set_props(thing, dmap)
        return thing


class Elem(metaclass=ElemMeta):
    def __init__(self, db, id):
        self._db = db
        self._id = id

    @property
    def id(self):
        return self._id

    def set_props(self, **props):
        event = Event(self._db)

        dmap = {
            '_t': TASK_PROPS_SET,
            '_i': self.id,
        }
        thing_map_props(event, dmap, props)

        event.tasks.append(dmap)
        return event

    def _id_map(self):
        return {}

    async def fetch(self):
        dmap = await self._db._client._req_thing({self._db._target: self._id})
        return Elem(self._db, dmap)

    def __repr__(self):
        return '<{}:{} at {}>'.format(
            self.__class__.__name__,
            self._id,
            hex(id(self)))
