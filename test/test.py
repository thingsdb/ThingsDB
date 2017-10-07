import asyncio
import logging

class _RqlProtocol(asyncio.Protocol):

    _connected = False



    def connection_made(self, transport):
        '''
        override asyncio.Protocol
        '''

        logging.info('made')

    def connection_lost(self, exc):
        '''
        override asyncio.Protocol
        '''
        logging.info('lost')

    def data_received(self, data):
        '''
        override asyncio.Protocol
        '''
        logging.info('received')

    


if __name__ == '__main__':
    loop = asyncio.get_event_loop()


    client = loop.create_connection(
        _RqlProtocol,
        host='localhost',
        port=9220)

    loop.run_until_complete(
            asyncio.wait_for(client, timeout=5))