import struct
import qpack


class Package(object):

    __slots__ = ('pid', 'length', 'total', 'tp', 'checkbit', 'data')

    st_package = struct.Struct('<IHBB')

    def __init__(self, barray):
        self.length, self.pid, self.tp, self.checkbit = \
            self.__class__.st_package.unpack_from(barray, offset=0)
        self.total = self.__class__.st_package.size + self.length
        self.data = None

    def extract_data_from(self, barray):
        try:
            self.data = qpack.unpackb(
                barray[self.__class__.st_package.size:self.total],
                decode='utf8') \
                if self.length else None
        finally:
            del barray[:self.total]

    def __repr__(self):
        return '<id: {0.pid} size: {0.length} tp: {0.tp}>'.format(self)