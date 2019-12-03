#!/usr/bin/env python3

import socket
import msgpack

data = msgpack.packb(["@t", "1 + 1;"])

print('Data', repr(data), ' Len', len(data))

HOST = '127.0.0.1'  # The server's hostname or IP address
PORT = 9200         # The port used by the server

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(b'\x17\x00\x00\x00\x00\x00\x21\xde\xb6RzDFlsoucQfDqrwrfGGEtc')
    data = s.recv(1024)

    print('Received', repr(data))

    s.sendall(b'\x0b\x00\x00\x00\x00\x00\x22\xdd\x92\xa2@t\xa61 + 1;')
    data = s.recv(1024)

    print('Received', repr(data))
