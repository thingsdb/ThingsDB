import time
import asyncio
import logging
import pprint
import signal
from thingsdb.client import Client
from thingsdb.exceptions import ThingsDBError

'''

from thingsdb.client import Client
from thingsdb.exceptions import ThingsDBError
client = Client()
await client.connect('localhost', 9200)
await client.authenticate('iris', 'siri')
collection = await client.get_collection('Test')

await collection.watch()
await collection.assign('y', 123)

'''

interrupted = False


class Thing:
    pass


class Collection(Thing):
    pass


class Label(Thing):
    name = TString()
    description = TString()


class Labels(Thing):
    vec = TArrayOf(Label)


class Conditions(Thing):
    labels = TArrayOf(Label)


class OsData(Collection):
    labels = Labels
    conditions = Conditions


async def test():
    client = Client()
    await client.connect('localhost', 9200)

    # Authenticte
    try:
        await client.authenticate('admin', 'pass')
    except ThingsDBError as e:
        print(e)

    client.use('osdata')

    start = time.time()
    try:
        res = await client.query(r'''
        /*
         * test query
         */
        /* del_collection('test2'); */
        collections();
        /* set_loglevel(WARNING); */

        ''', timeout=2, target=0)
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        print('Time: {}'.format(time.time() - start))
        pprint.pprint(res)

    print('-----------------------------------------------------------------')

    start = time.time()
    try:
        res = await client.query(r'''

        /*
        Oversight.nodes.push({
            address: 'localhost',
            secret: 'secret001',
            port: 8721,
        });
        */

        $a = _ => _;

        Oversight.nodes.map($a);

        /*
        Oversight.nodes.map(n, i => n.secret = ("somesecret" + str(i)));
        */

        /*
        Labels.labels[0].name = '!!! New Name2 !!!';
        */

        /*
        Labels.labels.splice(-1, 1);
        */

        Labels.labels.map(_ => _);

        ''', blobs=["bla"], timeout=2)
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        print('Time: {}'.format(time.time() - start))
        pprint.pprint(res)

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
