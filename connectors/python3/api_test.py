#!/usr/bin/env python
import requests
import os
import json
import msgpack
import pprint

stuff = 'http://localhost:9210/collection/stuff'
thingsdb = 'http://localhost:9210/t'
fn = '/home/joente/Downloads/sqlsrv01.insignit.local.json'
fn = '/home/joente/Downloads/test.json'

with open(fn, 'r') as f:
    data = json.load(f)


data = msgpack.dumps({
    'type': 'query',
    'code': '"Hello world!";'
})

x = requests.post(
    thingsdb,
    data=data,
    auth=('admin', 'pass'),
    headers={'Content-Type': 'application/msgpack'}
)

if x.status_code == 200:
    content = msgpack.unpackb(x.content, raw=False)
    pprint.pprint(content)
else:
    print(x.text)


# x = requests.post(
#     'http://localhost:9210/thingsdb',
#     json={
#         'type': 'query',
#         'query': "new_node('bla', '127.0.0.1', 9221);"
#     },
#     auth=('admin', 'pass')
# )

# if x.status_code == 200:
#     pprint.pprint(x.text)
# else:
#     print(x.text)

x = requests.post(
    stuff,
    json={
        'type': 'query',
        'code': ".iiris.age = age;",
        'vars': {
            'age': 6
        }
    },
    headers={'Authorization': 'TOKEN BerXvDO5BHkPybYyA55A3m'}
)

if x.status_code == 200:
    j = x.json()
    print(j)
else:
    print(x.text)


x = requests.post(
    stuff,
    json={
        'type': 'run',
        'procedure': 'addone',
        'args': [411213254564564]
    },
    auth=('admin', 'pass')
)

if x.status_code == 200:
    j = x.json()
    print(j)
else:
    print(x.status_code)
    print(x.text)


x = requests.post(
    stuff,
    json={
        'type': 'query',
        'code': '.addone',
        'args': [123]
    },
    auth=('admin', 'pass')
)

if x.status_code == 200:
    j = x.json()
    print(j)
else:
    print(x.status_code)
    print(x.text)
