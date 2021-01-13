import os
import struct
import msgpack


PROTO_EXT_INIT = 64
PROTO_EXT_CLOSE = 65
PROTO_EXT_REQ = 66
PROTO_EXT_RES = 67


class Package(object):

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



def make_pkg(pid, tp, data=None, is_bin=False):
    data = data if is_bin else b'' if data is None else \
        msgpack.packb(data, use_bin_type=True)

    header = Package.st_package.pack(
        len(data),
        pid,
        tp,
        tp ^ 0xff)

    return header + data



if __name__ == '__main__':
    pkg = make_pkg(0, PROTO_EXT_CLOSE)

    # write pkg to stdout
    os.sys.stdout.buffer.write(pkg)
    os.sys.stdout.buffer.flush()

