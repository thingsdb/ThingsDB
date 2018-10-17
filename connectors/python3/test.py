import asyncio
import logging
import tin.event
from tin.client import Client
from tin.thing import Elem


class Person(Elem):
    name = str


async def test():
    import time
    client = Client()
    await client.connect('localhost')

    # Authenticte
    await client.authenticate('iris', 'siri')

    db = await client.get_database('dbtest')
    print(dir(db.person))
    print(db.person)
    await db.person.fetch()
    print(dir(db.person))
    # print(a.age)

    # sasientje = db.new_thing(name='Sasientje')
    # iriske = db.new_thing(name='Iriske')

    # event = tin.event.join(
    #     sasientje.set_props(d=iriskem),
    #     iriske.set_props(m=sasientje))

    # await db.person.set_props(friend=db.new_thing(name='Sasientje')).apply()
    print(db.person.age)
    print(db.person.who)
    print(db.person.me.me.me.who)

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

    # # Read database root thingent
    # start = time.time()
    # res = await client.get_thing({
    #     'dbtest': -1,
    # })
    # root = Root(res)
    # root_id = res['_i']
    # end = time.time() - start
    # print('Get root thing result:', res, ' in ', end)

    # # Set properties
    # res = await client.trigger({
    #     'dbtest': [{
    #         '_t': TASK_PROPS_SET,
    #         '_i': root_id,
    #         'bla': 'bla'
    #     }]
    # })
    # print('Set props result:', res)

    # # Add an set a new thingent
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
    # print('New thing result:', res)
    #
    #


if __name__ == '__main__':
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    loop = asyncio.get_event_loop()
    loop.run_until_complete(test())