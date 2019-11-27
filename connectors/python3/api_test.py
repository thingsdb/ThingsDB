#!/usr/bin/env python
import requests
import os
import json
import msgpack

url = 'http://localhost:9210/collection/1'
fn = '/home/joente/Downloads/sqlsrv01.insignit.local.json'
fn = '/home/joente/Downloads/test.json'

with open(fn, 'r') as f:
    data = json.load(f)


data = msgpack.dumps({
    'type': 'quer',
    'query': '.keys();'
})

x = requests.post(
    url,
    data=data,
    auth=('admin', 'pass'),
    headers={'Content-Type': 'application/msgpack'}
)

print(x)
print(x.content)
