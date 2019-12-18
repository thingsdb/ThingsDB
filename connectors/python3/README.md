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


## Client

### thingsdb.client.Client 

```python3
thingsdb.client.Client(
    auto_reconnect: bool = True, 
    ssl: Optional[Union[bool, ssl.SSLContext]] = None, 
    loop: Optional[asyncio.AbstractEventLoop] = None
) -> Client
```
Initialize a ThingsDB client

- Args:
    - *auto_reconnect (bool, optional)*:
        When set to `True`, the client will automatically
        reconnect when a connection is lost. If set to `False` and the
        connection gets lost, one may call the `reconnect()` method to
        make a new connection. Defaults to True.
    - *ssl (SSLContext or bool, optional)*:
        Accepts an ssl.SSLContext for creating a secure connection
        using SSL/TLS. This argument may simply be set to `True` in
        which case a context using `ssl.PROTOCOL_TLS` is created.
        Defaults to None.
    - *loop (AbstractEventLoop, optional)*:
        Can be used to run the client on a specific event loop.
        If this argument is not used, the default event loop will be
        used. Defaults to None.


## Model

It is possible to create a model which will map to data in ThingsDB.
The model will be kept up-to-date be the client. It is possible to break
anywhere you want in the model. What is not provided, will not be watched.



