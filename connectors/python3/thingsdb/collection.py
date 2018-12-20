from .thing import Thing


class Collection(Thing):

    def __init__(self, client, collection_id):
        self.client = client
        self.collection_id = collection_id
        self._watching = set()
        super().__init__(self, collection_id)
