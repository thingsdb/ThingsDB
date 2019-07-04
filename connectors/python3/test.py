import time
import asyncio
import logging
import pprint
import signal
from thingsdb.client import Client
from thingsdb.client.scope import Scope
from thingsdb.exceptions import ThingsDBError
from thingsdb.exceptions import IndexError
from thingsdb.model import (
    Thing, Collection, array_of, set_of, required, optional
)


interrupted = False
osdata = None

"""

requires GRANT

add_token('iris', nil, 'used for bla bla')

token: x-charater str
issue_date:  ts
expire_date: nil


tokens


missing functions:
  isnil --> returns true if the given value == nil
  isthing --> return `true` if the given value is a `thing`
  type  --> returns a string for the given type
  set   --> create a new `set` value, or convert from given array
  add   --> adds a thing to a set
  has   --> returns `true` if is contains a given thing or id
  find  --> on set, reurns the `first` thing when a given closure
            evaluates to true
  pop   --> on set, some "random" thing is popped, or by a given thing or id
            and `nil` is re
  remove--> on set, by closure
  array --> convert to array (al least a `set`)



client.new_handle(
    name='new_label',
    args=['$name', '$description'],
    code=r'''
        assert(isstr($name), '$name must be a string');
        assert(isstr($description), '$description must be a string');
        assert(
            isnil(labels.find(|label| (label.name == $name)),
            '$name mst be unique'
        );
        labels.push({
            name: $name,
            description: $description,
        });
''')
"""


class Label(Thing):

    name = optional(str)
    description = optional(str)

    def get_name(self):
        return self.name if self else 'unknown'

    async def on_update(self, event_id, jobs):
        await super().on_update(event_id, jobs)
        print('on update: ', jobs)


class Condition(Thing):
    name = str
    description = str
    labels = array_of(Label)


class Host(Thing):
    name = str


class Hosts(Thing):
    vec = array_of(Host)


class OsData(Collection):
    labels = required(array_of(Label))
    ulabels = required(set_of(Label))
    conditions = required(array_of(Condition))
    other = required(list)
    hosts = Hosts
    name = optional(str)
    counter = required(int)


async def setup_initial_data(client, collection):
    client.use(collection)
    await client.query(r'''
        ulabels.add(
            {name: "Label1"},
            {name: "Label2"},
            {name: "Label3"},
        );
    ''')


async def test(client):
    global osdata

    await client.connect_pool([
        ('localhost', 9200),
        ('localhost', 9201),
    ], 'admin', 'pass')

    try:
        osdata = OsData(client, build=setup_initial_data)

        print(await client.node())

        while True:
            if interrupted:
                break

            if osdata:
                print(osdata.ulabels)
                print([label.name for label in osdata.ulabels if label])
            await asyncio.sleep(1.2)
    finally:
        client.close()


def signal_handler(signal, frame):
    print('Quit...')
    global interrupted
    interrupted = True


if __name__ == '__main__':
    client = Client()
    signal.signal(signal.SIGINT, signal_handler)

    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test(client))
    loop.run_until_complete(client.wait_closed())
    print('-----------------------------------------------------------------')
