import time
import asyncio
import logging
import pprint
import signal
from thingsdb.client import Client
from thingsdb.client.scope import Scope
from thingsdb.exceptions import ThingsDBError
from thingsdb.exceptions import IndexError
from thingsdb.model import Thing, Collection, array_of


interrupted = False
osdata = None

"""

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

    name = str
    description = str

    def get_name(self):
        return self.name if self else 'unknown'

    async def on_update(self, event_id, jobs):
        await super().on_update(event_id, jobs)
        print('on update: ', jobs)


class Condition(Thing):

    name = str
    description = str
    labels = array_of(Label)


class OsData(Collection):

    labels = array_of(Label)
    conditions = array_of(Condition)
    other = array_of(Thing)


async def test():
    global osdata
    client = Client()

    await client.connect('localhost', 9200)
    await client.authenticate('admin', 'pass')

    osdata = OsData(client)

    print(await client.node())

    while True:
        if interrupted:
            break

        if osdata:
            print([label.name for label in osdata.labels if label])
            print([str(x) for x in osdata.other])
        await asyncio.sleep(1.2)

    print('-----------------------------------------------------------------')


def signal_handler(signal, frame):
    print('Quit...')
    global interrupted
    interrupted = True


if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal_handler)

    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test())
