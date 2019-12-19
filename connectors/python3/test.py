import time
import warnings
import asyncio
import logging
import pprint
import pickle
import signal
import ssl
from thingsdb.client import Client
from thingsdb.model import Collection, Thing, ThingStrict


def PropType(b):
    return b


class Book(Thing):
    title = 'str'
    me = 'Book?', lambda: Book


class Bar(ThingStrict):
    bar = 'str'


class Stuff(Collection):
    __COLLECTION_NAME__ = 'stuff'

    book = 'Book', Book
    bar = 'Bar', Bar
    other = 'Thing?'


interrupted = False

setup_code = '''
.book = Book{
    title: 'Some example book',
};
.bar = Bar{
    bar: 'Chocolate bar',
};
.other = nil;
'''


async def test(client):
    global osdata

    # await client.connect('35.204.223.30', port=9400)
    # await client.authenticate('aoaOPzCZ1y+/f0S/jL1DUB')  # admin
    # await client.authenticate('V1CsgMetJcOHlqPGCigitz')  # Kolnilia
    # client.set_default_scope('//Kolnilia')

    await client.connect('localhost', port=9200)
    await client.authenticate('admin', 'pass')

    stuff = Stuff()
    await stuff.build(client, scripts=[setup_code], delete_if_exists=True)
    await stuff.load(client)

    try:
        res = await client.query('''
            "Hello ThingsDB!";
        ''')
        pprint.pprint(res)

        while not interrupted:
            if hasattr(stuff, 'book'):
                if hasattr(stuff.book, 'title'):
                    print(stuff.book.title)

            await asyncio.sleep(0.5)

        # res = await client.query('''
        #     nodes_info();
        # ''', scope='@n')
        # pprint.pprint(res)

        # res = await client.run('new_playground', 'Kolnilia', scope='@t')
        # pprint.pprint(res)

    finally:
        client.close()


def signal_handler(signal, frame):
    print('Quit...')
    global interrupted
    interrupted = True


if __name__ == '__main__':
    # client = Client(ssl=ssl.SSLContext(ssl.PROTOCOL_TLS))
    client = Client()
    signal.signal(signal.SIGINT, signal_handler)

    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test(client))
    loop.run_until_complete(client.wait_closed())
    print('-----------------------------------------------------------------')
