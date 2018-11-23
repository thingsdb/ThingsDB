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
        await client.authenticate('iris', 'siri')
    except ThingsDBError as e:
        print(e)

    client.use('dbtest')

    start = time.time()
    try:
        res = await client.watch([3])
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        print(time.time() - start)
        pprint.pprint(res)

    start = time.time()
    try:
        res = await client.query(r'''
        /*
         * test query
         */

        people.filter(x => x.name.lower().endswith('ris'));

        a = {name: 'Iris', age: 5};

        a.filter(k, v => (v == 'Iris'));

        people;





        ''', blobs=["bla"], timeout=2)
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        print(time.time() - start)
        pprint.pprint(res)

    start = time.time()
    try:
        res = await client.query(r'''
        c.del();
        ''', blobs=["bla"], timeout=2)
    except ThingsDBError as e:
        print(f"{e.__class__.__name__}: {e}")
    else:
        # print(time.time() - start)
        pprint.pprint(res)

    await asyncio.sleep(1)



if __name__ == '__main__':
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test())
