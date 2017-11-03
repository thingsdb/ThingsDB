

class Event:
    def __init__(self, db):
        self._db = db
        self.tasks = []

    async def apply(self):
        await self._client.trigger({self._db._target: self.tasks})


