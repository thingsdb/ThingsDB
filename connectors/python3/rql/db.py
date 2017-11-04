from .elem import Elem
from .elem import elem_set_props
from .elem import elem_map_props
from .event import Event
from .protocol import TASK_PROPS_SET


class Db(Elem):
    def __init__(self, client, target, dmap):
        self._client = client
        self._target = target
        self._elems = {}
        self._next_id = 0
        id = dmap.pop('_i')
        super().__init__(self, id)
        elem_set_props(self, dmap)

    def new_elem(self, **props):
        event = Event(self._db)

        dmap = {
            '_t': TASK_PROPS_SET,
            '_i': self.new_id,
        }

        elem_map_props(dmap, props)
        event.tasks.append(dmap)
        return event

    @property
    def new_id(self):
        self._next_id += 1
        return -self._next_id
