from .version import __version__
from .wrap import wrap
from .client import exceptions

THINGSDB = {'thingsdb'}
NODE = {'node'}


def fmt(val, blobs=None):
    if blobs is None:
        blobs = []

    return repr(wrap(val, blobs))
