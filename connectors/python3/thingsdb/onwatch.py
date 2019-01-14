class OnWatch:

    @staticmethod
    async def on_init(thing, thing_dict):
        thing.on_watch_init(thing_dict)

    @staticmethod
    async def on_update(thing, jobs):
        thing.on_watch_update(jobs)

    @staticmethod
    async def on_delete(thing):
        thing.on_watch_delete()
