# Python connector and ORM for ThingsDB

> This library requires Python 3.6 or higher.

---------------------------------------

  * [Installation](#installation)
  * [Quick usage](#quick-usage)
  * [Client](#client)
    * [Client()](thingsdb.client.Client)
    * [query](thingsdb.client.Client.query)
  * [Model](#model)
    * [Collection](#collection)
    * [Thing](#thing)

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

This is an client using `asyncio` which can be used for running queries to
ThingsDB.


### thingsdb.client.Client

```python
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

### thingsdb.client.Client.query

```python
await query(
        code: str,
        scope: Optional[str] = None,
        timeout: Optional[int] = None,
        convert_vars: bool = True,
        **kwargs: Any
) -> Any
```

Query ThingsDB.

Use this method to run `code` in a scope.

- Args:
    - *code (str)*:
        ThingsDB code to run.
    - *scope (str, optional)*:
        Run the code in this scope. If not specified, the default scope
        will be used. See https://docs.thingsdb.net/v0/overview/scopes/
        for how to format a scope.
    - *timeout (int, optional)*:
        Raise a time-out exception if no response is received within X
        seconds. If no time-out is given, the client will wait forever.
        Defaults to None.
    - *convert_vars (bool, optional)*:
        Only applicable if `**kwargs` are given. If set to True, then
        the provided **kwargs values will be converted so ThingsDB can
        understand them. For example, a thing should be given just by
        it's ID and with conversion the `#` will be extracted. When
        this argument is False, the **kwargs stay untouched.
        Defaults to True.
    - *\*\*kwargs (any, optional)*:
        Can be used to inject variable into the ThingsDB code.

#### Examples

Although we could just as easy have wrote everything in the
ThingsDB code itself, this example shows how to use **kwargs for
injecting variable into code. In this case the variable `book`.

```python
res = await client.query(".my_book = book;", book={
    'title': 'Manual ThingsDB'
})
```

#### Returns

The result of the ThingsDB code.

> If the ThingsDB code will return with an exception, then this
> exception will be translated to a Python Exception which will be
> raised. See thingsdb.exceptions for all possible exceptions and
> https://docs.thingsdb.net/v0/errors/ for info on the error codes.

## Model

It is possible to create a model which will map to data in ThingsDB.
The model will be kept up-to-date be the client. It is possible to break
anywhere you want in the model. What is not provided, will not be watched.

### Collection

A collection is always required, even you do not plan to watch anything in the
root of the collection. In the latter case you can just create an empty
collection which can be used when initializing individual things.

```python
import asyncio
from thingsdb.client import Client
from thingsdb.model import Collection

class Foo(Collection):
    name = 'str'
```

### Thing

```python
import asyncio
from thingsdb.client import Client
from thingsdb.model import Collection, Thing

class Bar(Thing):
    name = 'str'
    other = 'Bar', lambda: Bar

class Foo(Collection):
    bar: 'Bar', Bar

async def example():
    client = Client()
    foo = Foo()
    await client.connect('localhost')
    try:
        await client.authenticate('admin', 'pass')
        await foo.load(client)

        # ... now the collection will be watched

    finally:
        client.close()
        await client.wait_closed()
```

Suppose you have an ID and want to watch that single thing, then
you can initialize the thing and call `watch()` manually. For example,
consider we have an `#5` for a `Bar` type in collection `Foo`:

```python
bar = Bar(foo, 5)
await bar.watch()
```
