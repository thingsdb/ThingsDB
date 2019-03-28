from .version import __version__
from .wrap import wrap


def fmt(val, blobs=None):
    if blobs is None:
        blobs = []

    return wrap(val, blobs)
