def join(*events):
    event = events[0]
    for e in events[1:]:
        if e._db != event._db:
            raise ValueError('all events must have the same target database')
        event.tasks += e.tasks
    return event


class Event:
    def __init__(self, db):
        self._db = db
        self.tasks = []

    async def apply(self):
        await self._db._client._req_event({self._db._target: self.tasks})

