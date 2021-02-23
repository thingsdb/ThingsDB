import struct
import msgpack


class Pkg(object):

    __slots__ = ('pid', 'length', 'total', 'tp', 'checkbit', 'data')

    st_package = struct.Struct('<IHBB')

    def __init__(self, barray: bytearray) -> None:
        self.length, self.pid, self.tp, self.checkbit = \
            self.__class__.st_package.unpack_from(barray, offset=0)
        self.total = self.__class__.st_package.size + self.length
        self.data = None

    def extract_data_from(self, barray: bytearray) -> None:
        try:
            self.data = msgpack.unpackb(
                barray[self.__class__.st_package.size:self.total],
                raw=False) \
                if self.length else None
        finally:
            del barray[:self.total]

    def __repr__(self) -> str:
        return '<id: {0.pid} size: {0.length} tp: {0.tp}>'.format(self)
