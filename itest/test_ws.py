import msgpack
import ssl
import struct
from websockets.sync.client import connect


st_package = struct.Struct('<IHBB')

context = ssl._create_unverified_context()

def example():
    with connect("wss://localhost:7681", ssl_context=context) as websocket:
        data = msgpack.packb(['admin', 'pass'])
        header = st_package.pack(
            len(data),
            0,
            33,
            33 ^ 0xff)
        websocket.send(header+data)
        resp = websocket.recv()
        data = msgpack.packb(['//stuff', """//ti
            6 * 7;
        """])
        header = st_package.pack(
            len(data),
            0,
            34,
            34 ^ 0xff)
        websocket.send(header+data)
        resp = websocket.recv()
        answer = msgpack.unpackb(resp[8:])
        print(answer)

example()