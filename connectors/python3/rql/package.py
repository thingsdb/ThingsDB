import struct


class Package(object):

    __slots__ = ('pid', 'length', 'total', 'tipe', 'checkbit', 'data')

    struct_datapackage = struct.Struct('<IHBB')

    def __init__(self, barray):
        self.length, self.pid, self.tipe, self.checkbit = \
            self.__class__.struct_datapackage.unpack_from(barray, offset=0)
        self.total = self.__class__.struct_datapackage.size + self.length
        self.data = None

    def extract_data_from(self, barray):
        try:
            self.data = qpack.unpackb(
                barray[self.__class__.struct_datapackage.size:self.total],
                decode='utf8') \
                if self.length else None
        finally:
            del barray[:self.total]

    def __repr__(self):
        return '<id: {0.pid} size: {0.length} tp: {0.tipe}>'.format(self)