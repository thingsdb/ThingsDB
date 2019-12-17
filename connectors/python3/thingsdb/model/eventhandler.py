from ..client.abc.events import Events


class EventHandler(Events):

    def __init__(self, collection):
        super().__init__()
        self._collection = collection

    def on_reconnect(self):
        self._collection.on_reconnect()

    def on_node_status(self, _status):
        pass

    def on_warning(self, _warn):
        pass

    def on_watch_init(self, data):
        thing_dict = data['thing']
        thing_id = thing_dict.pop('#')
        thing = self._collection._things.get(thing_id)
        if thing is None:
            thing_dict['#'] = thing_id  # restore `thing_dict`
            logging.debug(
                f'Cannot init #{thing_id} since the thing is not registerd '
                f'for watching by collection `{self._collection._name}`')
            return

        thing.on_init(data['event'], thing_dict)

    def on_watch_update(self, data):
        thing_id = data['#']
        thing = self._collection._things.get(thing_id)
        if thing is None:
            logging.debug(
                f'Cannot update #{thing_id} since the thing is not registerd '
                f'for watching by collection `{self._collection._name}`')
            return

        thing.on_update(data['event'], data.pop('jobs'))

    def on_watch_delete(self, data):
        thing_id = data['#']
        thing = self._collection._things.get(thing_id)
        if thing is None:
            logging.debug(
                f'Cannot update #{thing_id} since the thing is not registerd '
                f'for watching by collection `{self._collection._name}`')
            return

        thing.on_delete()
