class Elem:
    def __init__(self, db, data):
        self._db = db
        self._id = data.pop('_i')

    @property
    def id(self):
        return self._id

    def set_props(self, *pairs):
        event = Event(self._db)
        event.tasks.append({
            '_t': TASK_PROPS_SET,
            '_i': self.id,

        })