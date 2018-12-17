import time
import asyncio
import logging
import pprint
import signal
from thingsdb.client import Client
from thingsdb.exceptions import ThingsDBError


interrupted = False


async def test():
    client = Client()
    await client.connect('localhost', 9200)

    # Authenticte
    try:
        await client.authenticate('iris', 'siri')
    except ThingsDBError as e:
        print(e)

    client.use('test')

    start = time.time()
    try:
        res = await client.watch([3])
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        print('Time: {}'.format(time.time() - start))
        pprint.pprint(res)

    await asyncio.sleep(0.5)
    print('-----------------------------------------------------------------')

    start = time.time()
    try:
        res = await client.query(r'''
        /*
         * test query
         */
        /* del_collection('test2'); */
        node();
        nodes();
        counters();
        /* new_node('secret', '127.0.0.1', 9221); */
        /* new_collection('test'); */
        collections();
        users();

        /* set_loglevel(WARNING); */

        ''', timeout=2, target=0)
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        print('Time: {}'.format(time.time() - start))
        pprint.pprint(res)

    await asyncio.sleep(0.5)
    print('-----------------------------------------------------------------')

    start = time.time()
    try:
        res = await client.query(r'''

        {name: 'iris'}.find(x=>x);

        ''', blobs=["bla"], timeout=2)
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        print('Time: {}'.format(time.time() - start))
        pprint.pprint(res)

    while not interrupted:
        await asyncio.sleep(0.1)

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
