#!/usr/bin/env python
import requests
import os
import json

url = 'http://localhost:9210/collection/1'
fn = '/home/joente/Downloads/sqlsrv01.insignit.local.json'
fn = '/home/joente/Downloads/test.json'

with open(fn, 'r') as f:
    data = json.load(f)



x = requests.post(url, json=data, auth=('admin', 'pass'))
