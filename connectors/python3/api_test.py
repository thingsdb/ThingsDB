#!/usr/bin/env python
import requests

url = 'http://localhost:9210/collection/1'

data = {
    'somekey': 'somevalue'
}

x = requests.post(url, data=data)
