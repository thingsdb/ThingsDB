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

try($tmp = procedure_def($name));


new_procedure("
set_procedure ($name)

new_procedure("

replace_procedure ($old, $def) {
    'Replaces procedure `old` with a new procedure definition given by `def`';

    /* Backup old procedure */
    $backup = procedure_def($old);

    /* Delete old procedure */
    del_procedure($old);

    /* TODO: Here we actually want to catch the error  as ti_val_t */
    $ret = try( new_procedure($def) );

    (iserror($ret)) ? {
        new_procedure($backup);
        raise ($ret);
    } : $ret;
}


");

TODO: exception
error as ti_val_t

- Remove `alt` in try, but return error instead.
- Only `try` should convert ex_t to a ex_t value as query result.
- Create function `raise( error ) which puts an error back to ex_t
-
- Maybe add statement blocks?
- Add functions like index_error(), error(code > 0, string)
  (build-in error < 0)
- Change proto to a general error proto, and force the client to act on
  code/message.
- store errors like {'!': msg, 'error_code': x}


assert()
error('bla bla', [code=1])   code 1..32
overflow_error()
equal between error, -> compare error_code



")

TODO: docs
- procedures   (general overview)
- call-request (call request explained)
- f_call
- f_new_procedure
- f_del_procedure
- f_rename_procedure ???
- f_procedure_def
- f_procedure_doc
- f_procedure_fmt ?? --> format a procedure?  this can work in place!!
- deep  (change to syntax => deep)

TODO: root with `.`
access the collection root with a `.`
then we can remove $variable

introduce `wse`
then we can remove `calle`



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


class MyCollection(Collection):
    greet = required(str)


'''
Hello, welcome to ThingsDB

'''


async def test(client):
    global osdata

    await client.connect('localhost')
    await client.authenticate(['admin', 'pass'])

    try:
        x = await client.run('pp', 4, target='stuff')
        print(x)
        # my_collection = MyCollection(client, build=True)
        # x = await client.run('addone', 10, target='stuff')
        # print(x)
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
