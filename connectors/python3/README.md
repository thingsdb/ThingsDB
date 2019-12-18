# Python connector and ORM for ThingsDB

> This library requires Python 3.6 or higher.

---------------------------------------
  * [Installation](#installation)
  * [Quick usage](#quick-usage)
  * [Client](#client)

---------------------------------------

## Installation

Just use pip:

```
pip install python-thingsdb
```

Or, clone the project and use setup.py:

```
python setup.py install
```

## Quick usage

This is an async client which can be used for running queries to ThingsDB.
It required

```python
import asyncio
from thingsdb.client import Client

async def hello_world():
    client = Client()

    # replace `localhost` with your ThingsDB server address
    await client.connect('localhost')

    try:
        # replace `admin` and `pass` with your username and password
        # or use a valid token string
        await client.authenticate('admin', 'pass')

        # perform the hello world code...
        print(await client.query('''
            "Hello World!";
        ''')

    finally:
        # the will close the client in a nice way
        client.close()
        await client.wait_closed()

# run the hello world example
asyncio.get_event_loop().run_until_complete(hello_world())
```

## Model

It is possible to create a model which will map to data in ThingsDB.
The model will be kept up-to-date be the client. It is possible to break
anywhere you want in the model. What is not provided, will not be watched.



