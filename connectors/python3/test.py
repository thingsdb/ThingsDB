import asyncio
import logging
from rql.client import Client


async def test():
    import time
    client = Client()
    await client.connect('localhost')

    # Authenticte
    res = await client.authenticate('iris', 'siri')
    print('Auth result:', res)

    # # Create user
    # res = await client.trigger({
    #     '_': [{
    #         '_t': TASK_USER_CREATE,
    #         '_u': 'iriss',
    #         '_p': 'siri'
    #     }]
    # })
    # print('Create user result:', res)

    # # Create database
    # res = await client.trigger({
    #     '_': [{
    #         '_t': TASK_DB_CREATE,
    #         '_u': 'iris',
    #         '_n': 'dbtest'
    #     }]
    # })
    # print('Create database result:', res, type(res))

    # # Read database root element
    # start = time.time()
    # res = await client.get_elem({
    #     'dbtest': -1,
    # })
    # root = Root(res)
    # root_id = res['_i']
    # end = time.time() - start
    # print('Get root elem result:', res, ' in ', end)

    # # Set properties
    # res = await client.trigger({
    #     'dbtest': [{
    #         '_t': TASK_PROPS_SET,
    #         '_i': root_id,
    #         'bla': 'bla'
    #     }]
    # })
    # print('Set props result:', res)

    # # Add an set a new element
    # res = await client.trigger({
    #     'dbtest': [{
    #         '_t': TASK_PROPS_SET,
    #         '_i': -1,
    #         'who': 'me'
    #     }, {
    #         '_t': TASK_PROPS_SET,
    #         '_i': root_id,
    #         'person': {'_i': -1}
    #     }]
    # })
    # print('New elem result:', res)


if __name__ == '__main__':
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test())