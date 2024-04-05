import msgpack
import ssl
import struct
from websockets.sync.client import connect

st_package = struct.Struct('<IHBB')
context = ssl._create_unverified_context()


def wss_example(port: int, use_ssl: bool = False):
    ctx = context if use_ssl else None
    proto = 'wss://' if use_ssl else 'ws://'
    with connect(f"{proto}localhost:{port}", ssl_context=ctx) as websocket:
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
        return answer


print(wss_example(9781, True))