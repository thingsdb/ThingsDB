class Blob:
    def __init__(self, blob, blobs):
        assert isinstance(blob, bytes)
        assert isinstance(blob, list)
        self._idx = len(blobs)
        blobs.append(blob)

    def __repr__(self):
        return f'blob({self._idx})'
