from .protocol import REQ_QUERY_COLLECTION


class Target:

    def __init__(self, target: str, _proto=REQ_QUERY_COLLECTION):
        self._target = target
        self._proto = _proto

    def is_collection(self) -> bool:
        return self._proto == REQ_QUERY_COLLECTION

    def name(self):
        if isinstance(self._target, int):
            return f'collection:{self._target}'
        return self._target
