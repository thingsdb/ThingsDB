# Python connector and ORM for ThingsDB

> This library requires Python 3.6 or higher.

---------------------------------------

  * [Installation](#installation)
  * [Quick usage](#quick-usage)
  * [Client module](#client-module)
    * [Client()](#Client)
    * [authenticate](#authenticate)
    * [add_event_handler](#add_event_handler)
    * [close](#close)
    * [connect](#connect)
    * [connect_pool](#connect_pool)
    * [get_default_scope](#get_default_scope)
    * [get_event_loop](#get_event_loop)
    * [is_connected](#is_connected)
    * [query](#query)
    * [reconnect](#reconnect)
    * [run](#run)
    * [set_default_scope](#set_default_scope)
    * [unwatch](#unwatch)
    * [wait_closed](#wait_closed)
    * [watch](#watch)
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

## Client module

This is an client using `asyncio` which can be used for running queries to
ThingsDB.


### Client()

```python
thingsdb.client.Client(
    auto_reconnect: bool = True,
    ssl: Optional[Union[bool, ssl.SSLContext]] = None,
    loop: Optional[asyncio.AbstractEventLoop] = None
) -> Client
```
Initialize a ThingsDB client

#### Args

- *auto_reconnect (bool, optional)*:
    When set to `True`, the client will automatically
    reconnect when a connection is lost. If set to `False` and the
    connection gets lost, one may call the [reconnect()](#reconnect) method to
    make a new connection. Defaults to `True`.
- *ssl (SSLContext or bool, optional)*:
    Accepts an ssl.SSLContext for creating a secure connection
    using SSL/TLS. This argument may simply be set to `True` in
    which case a context using `ssl.PROTOCOL_TLS` is created.
    Defaults to `None`.
- *loop (AbstractEventLoop, optional)*:
    Can be used to run the client on a specific event loop.
    If this argument is not used, the default event loop will be
    used. Defaults to `None`.

### authenticate

```python
async Client().authenticate(
    *auth: Union[str, tuple],
    timeout: Optional[int] = 5
) -> None
```

Authenticate a ThingsDB connection.

#### Args

- *\*auth (str or (str, str))*:
    Argument `auth` can be be either a string with a token or a
    tuple with username and password. (the latter may be provided
    as two separate arguments
- *timeout (int, optional)*:
    Can be be used to control the maximum time in seconds for the
    client to wait for response on the authentication request.
    The timeout may be set to `None` in which case the client will
    wait forever on a response. Defaults to 5.

### add_event_handler

```python
Client().add_event_handler(event_handler: Events) -> None
```

Add an event handler.

Event handlers will called in the order they are added.

#### Args

- *event_handler (Events)*:
    An instance of Events (see `thingsdb.client.abc.events`).

### close

```python
Client().close() -> None
```

Close the ThingsDB connection.

This method will return immediately so the connection may not be
closed yet after a call to `close()`. Use the [wait_closed()](#wait_closed) method
after calling this method if this is required.

### connect

```python
async Client().connect(
    host: str,
    port: int = 9200,
    timeout: Optional[int] = 5
) -> asyncio.Future
```

Connect to ThingsDB.

This method will *only* create a connection, so the connection is not
authenticated yet. Use the [authenticate(..)](#authenticate) method after creating a
connection before using the connection.

#### Args

- *host (str)*:
    A hostname, IP address, FQDN to connect to.
- *port (int, optional)*:
    Integer value between 0 and 65535 and should be the port number
    where a ThingsDB node is listening to for client connections.
    Defaults to 9200.
- *timeout (int, optional)*:
    Can be be used to control the maximum time the client will
    attempt to create a connection. The timeout may be set to
    `None` in which case the client will wait forever on a
    response. Defaults to 5.

### Returns

Future which should be awaited. The result of the future will be
set to `None` when successful.

> Do not use this method if the client is already
> connected. This can be checked with `client.is_connected()`.

### connect_pool

```python
async Client().connect_pool(
    pool: list,
    *auth: Union[str, tuple]
) -> asyncio.Future
```

Connect using a connection pool.

When using a connection pool, the client will randomly choose a node
to connect to. When a node is going down, it will inform the client
so it will automatically re-connect to another node. Connections will
automatically authenticate so the connection pool requires credentials
to perform the authentication.

#### Examples

```python
await connect_pool([
    'node01.local',             # address as string
    'node02.local',             # port will default to 9200
    ('node03.local', 9201),     # ..or with an explicit port
], "admin", "pass")
```

#### Args

- *pool (list of addresses)*:
    Should be an iterable with node address strings, or tuples
    with `address` and `port` combinations in a tuple or list.
- *\*auth (str or (str, str))*:
    Argument `auth` can be be either a string with a token or a
    tuple with username and password. (the latter may be provided
    as two separate arguments

### Returns

Future which should be awaited. The result of the future will be
set to `None` when successful.

> Do not use this method if the client is already
> connected. This can be checked with `client.is_connected()`.


### get_default_scope

```python
Client().get_default_scope() -> str
```

Get the default scope.

The default scope may be changed with the [set_default_scope()](#set_default_scope) method.

#### Returns

The default scope which is used by the client when no specific scope is specified.


### get_event_loop

```python
Client().get_event_loop() -> asyncio.AbstractEventLoop
```

Can be used to get the event loop.

#### Returns

The event loop used by the client.

### is_connected

```python
Client().is_connected() -> bool
```

Can be used to check if the client is connected.

#### Returns
`True` when the client is connected else `False`.

### query

```python
Client().query(
        code: str,
        scope: Optional[str] = None,
        timeout: Optional[int] = None,
        convert_vars: bool = True,
        **kwargs: Any
) -> asyncio.Future
```

Query ThingsDB.

Use this method to run `code` in a scope.

#### Args

- *code (str)*:
    ThingsDB code to run.
- *scope (str, optional)*:
    Run the code in this scope. If not specified, the default scope
    will be used. See https://docs.thingsdb.net/v0/overview/scopes/
    for how to format a scope.
- *timeout (int, optional)*:
    Raise a time-out exception if no response is received within X
    seconds. If no time-out is given, the client will wait forever.
    Defaults to `None`.
- *convert_vars (bool, optional)*:
    Only applicable if `**kwargs` are given. If set to `True`, then
    the provided `**kwargs` values will be converted so ThingsDB can
    understand them. For example, a thing should be given just by
    it's ID and with conversion the `#` will be extracted. When
    this argument is `False`, the `**kwargs` stay untouched.
    Defaults to `True`.
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

Future which should be awaited. The result of the future will
contain the result of the ThingsDB code when successful.

> If the ThingsDB code will return with an exception, then this
> exception will be translated to a Python Exception which will be
> raised. See thingsdb.exceptions for all possible exceptions and
> https://docs.thingsdb.net/v0/errors/ for info on the error codes.

### reconnect

```python
async Client().reconnect() -> None
```

Re-connect to ThingsDB.

This method can be used, even when a connection still exists. In case
of a connection pool, a call to `reconnect()` will switch to another
node.


### run

```python
Client().run(
    procedure: str,
    *args: Optional[Any],
    scope: Optional[str] = None,
    timeout: Optional[int] = None,
    convert_args: bool = True
) -> asyncio.Future
```

Run a procedure.

Use this method to run a stored procedure in a scope.

#### Args

- *procedure (str)*:
    Name of the procedure to run.
- *\*args (any)*:
    Arguments which are injected as the procedure arguments. The
    number of args must match the number the procedure requires.
- *scope (str, optional)*:
    Run the procedure in this scope. If not specified, the default
    scope will be used.
    See https://docs.thingsdb.net/v0/overview/scopes/ for how to
    format a scope.
- *timeout (int, optional)*:
    Raise a time-out exception if no response is received within X
    seconds. If no time-out is given, the client will wait forever.
    Defaults to `None`.
- *convert_args (bool, optional)*:
    Only applicable if `*args` are given. If set to `True`, then
    the provided `*args` values will be converted so ThingsDB can
    understand them. For example, a thing should be given just by
    it's ID and with conversion the `#` will be extracted. When
    this argument is `False`, the `*args` stay untouched.
    Defaults to `True`.

#### Returns

Future which should be awaited. The result of the future will
contain the result of the ThingsDB procedure when successful.


> If the ThingsDB code will return with an exception, then this
> exception will be translated to a Python Exception which will be
> raised. See thingsdb.exceptions for all possible exceptions and
> https://docs.thingsdb.net/v0/errors/ for info on the error codes.


### set_default_scope

```python
Client().set_default_scope(scope: str) -> None
```

Set the default scope.

Can be used to change the default scope which is initially set to `@t`.

#### Args
- *scope (str)*:
    Set the default scope. A scope may start with either the `/`
    character, or `@`. Examples: `"//stuff"`, `"@:stuff"`, `"/node"`

### unwatch

```python
Client().unwatch(
    *ids: int,
    scope: Optional[str] = None
) -> asyncio.Future
```

Unsubscribe for changes on given things.

Stop receiving events for the things given by one or more ids. It is
possible that the client receives an event shortly after calling the
unsubscribe method because the event was queued.

#### Args
- *\*ids (int)*:
    Thing IDs to unsubscribe. No error is returned in case one of
    the given things are not found within the collection or if the
    thing was not being watched.
- *scope (str, optional)*:
    Unsubscribe for things in this scope. If not specified, the
    default scope will be used. Only collection scopes may contain
    things so only collection scopes can be used.
    See https://docs.thingsdb.net/v0/overview/scopes/ for how to
    format a scope.

#### Returns

Future which result will be set to `None` if successful.


### wait_closed

```python
async Client().wait_closed() -> None
```

Wait for a connection to close.

Can be used after calling the `close()` method to determine when the
connection is actually closed.


### watch

```python
Client().watch(self, *ids: int, scope: Optional[str] = None) -> asyncio.Future
```

Subscribe for changes on given things.

This method accepts one or more thing ids to subscribe to. This
method will simply return None as soon as the subscribe request is
successful handled by ThingsDB. After the response, the client will
receive `INIT` events for all subscribed ids. After that, ThingsDB
will continue to provide the client with `UPDATE` events which contain
changes to the subscribed thing. A `DELETE` event might be received
if, and only if the thing is removed and garbage collected from the
collection.

#### Args

- *\*ids (int)*:
    Thing IDs to subscribe to. No error is returned in case one of
    the given things are not found within the collection, instead a
    `WARN` event will be send to the client.
- *scope (str, optional)*:
    Subscribe on things in this scope. If not specified, the
    default scope will be used. Only collection scopes may contain
    things so only collection scopes can be used.
    See https://docs.thingsdb.net/v0/overview/scopes/ for how to
    format a scope.

#### Returns

Future which result will be set to `None` if successful.

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
