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


async def test():
    client = Client()
    await client.connect('localhost', 9200)

    # Authenticte
    try:
        await client.authenticate('iris', 'siri')
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
            secret: 'someblabla',
            port: 8721,
            in_sync: true
        }).ret();
        */


        /*
        Oversight.nodes.map(n, i => n.secret = ("somesecret" + str(i)));
        */

        Oversight.nodes.map(_ => _);


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
