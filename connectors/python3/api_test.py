#!/usr/bin/env python
import requests
import os
import json
import msgpack
import pprint

url = 'http://localhost:9210/node/1'
fn = '/home/joente/Downloads/sqlsrv01.insignit.local.json'
fn = '/home/joente/Downloads/test.json'

with open(fn, 'r') as f:
    data = json.load(f)


data = msgpack.dumps({
    'type': 'query',

    'query': 'node_info();'
})

x = requests.post(
    url,
    data=data,
    auth=('admin', 'pass'),
    headers={'Content-Type': 'application/msgpack'}
)

if x.status_code == 200:
    content = msgpack.unpackb(x.content, encoding='utf8')
    pprint.pprint(content)
else:
    print(x.text)


x = requests.post(
    url,
    json={'type': 'query', 'query': '5 5 5;'},
    auth=('admin', 'pass')
)

if x.status_code == 200:
    pprint.pprint(x.json)
else:
    print(x.text)
