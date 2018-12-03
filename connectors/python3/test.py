import time
import asyncio
import logging
import pprint
from thingsdb.client import Client
from thingsdb.exceptions import ThingsDBError


async def test():
    client = Client()
    await client.connect('localhost')

    # Authenticte
    try:
        await client.authenticate('sasha', 'iris')
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

        del_user('iris');

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

        [1,2,3];

        ''', blobs=["bla"], timeout=2)
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        print('Time: {}'.format(time.time() - start))
        pprint.pprint(res)

    await asyncio.sleep(0.5)
    print('-----------------------------------------------------------------')


if __name__ == '__main__':
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test())
