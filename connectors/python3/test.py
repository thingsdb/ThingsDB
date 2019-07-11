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


## Handlers

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

Requires: MODIFY

new_procedure(
    ".thingsdb",    /* collection or scope, ONLY thingsdb scope */
    "new_user",     /* procedure name */
    ["$name"],      /* arguments */
    "new_user($name); add_token($name);"    /* body */
)

del_procedure(
    ".thingsdb",     /* collection or socpe */
    "handler_name",  /* handler name */
)

rename_procedure(
    ".thingsdb",     /* collection or socpe */
    "handler_name",  /* handler name */
    "new_handler_name",  /* new_handler name */
)


Running requires CALL
[target, handle_name, args...]

procedure(
    ".thingsdb",
    "name",
)

procedures(
    ["collection_or_scope"]     /* if not given, .thingsdb scope */
);


"""

new_procudure("
new_user($name) {
    new_user($name)
    $key = new_token($name);
    grant(':thingsdb', $name, CALL);
    grant('OsData', $name, CALL);
    grant(':node', $name, WATCH);
    $key;
}
")



tiken_key = new_user('rik')




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

        print(await client.node_info())

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
