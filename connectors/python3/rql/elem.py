def elem_set_props(elem, dmap):
    for prop, val in dmap.items():
        assert (prop[0] != '_'), 'unexpected prop: {}'.format(prop)
        if isinstance(val, dict):
            val = Elem(elem._db, val)
        setattr(elem, prop, val)


class ElemMeta(type):
    def __call__(self, *args):
        if len(args) == 3:
            return super().__call__(*args)

        db, dmap = args
        id = dmap.pop('_i')
        elem = db._elems.get(id, super().__call__(db, id))
        elem_set_props(elem, dmap)

        return elem


class Elem(metaclass=ElemMeta):
    def __init__(self, db, id):
        self._db = db
        self._id = id

    @property
    def id(self):
        return self._id

    def set_props(self, *pairs):
        event = Event(self._db)
        event.tasks.append({
            '_t': TASK_PROPS_SET,
            '_i': self.id,
        })

    def __repr__(self):
        return '<{}:{} at {}>'.format(
            self.__class__.__name__,
            self._id,
            hex(id(self)))
