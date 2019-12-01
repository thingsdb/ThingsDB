#!/usr/bin/env python
import requests
import os
import json
import msgpack
import pprint

url = 'http://localhost:9210/collection/stuff'
fn = '/home/joente/Downloads/sqlsrv01.insignit.local.json'
fn = '/home/joente/Downloads/test.json'

with open(fn, 'r') as f:
    data = json.load(f)


data = msgpack.dumps({
    'type': 'query',

    'query': '"Hello world!";'
})

x = requests.post(
    url,
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
    url,
    json={
        'type': 'query',
        'query': ".iris.age = 6"
    },
    auth=('admin', 'pass')
)

if x.status_code == 200:
    pprint.pprint(x.text)
else:
    print(x.text)


x = requests.post(
    url,
    json={
        'type': 'run',
        'procedure': 'addone',
        'args': [5]
    },
    auth=('admin', 'pass')
)

if x.status_code == 200:
    j = x.json()
    print(j)
else:
    print(x.text)
