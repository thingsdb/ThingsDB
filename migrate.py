import msgpack

with open('collections.mp', 'rb') as f:

    data = msgpack.unpackb()
