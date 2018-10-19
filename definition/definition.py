import re
from pyleri import (
    Grammar,
    Keyword,
    Regex,
    Choice as Choice_,
    Sequence,
    Ref,
    Prio,
    Repeat,
    List,
    Optional,
    THIS,
)

class Choice(Choice_):
    def __init__(self, *args, most_greedy=None, **kwargs):
        if most_greedy is None:
            most_greedy = False
        super().__init__(*args, most_greedy=most_greedy, **kwargs)


class Definition(Grammar):
    RE_KEYWORDS = re.compile(r'^[$a-z][a-z_]*')

    r_single_quote_str = Regex(r"(?:'(?:[^']*)')+")
    r_double_quote_str = Regex(r'(?:"(?:[^"]*)")+')
    r_comment = Regex(r'\/\*.+?\*\/|\/\/.*(?=[\n\r])')

    k_create = Keyword('create')
    k_drop = Keyword('drop')
    k_rename = Keyword('rename')
    k_set = Keyword('set')
    k_fetch = Keyword('fetch')
    k_thing = Keyword('thing')
    k_map = Keyword('map')

    array_idx = Regex(r'-?[0-9]+')
    thing_id = Regex(r'[0-9]+')

    comment = Optional(Repeat(r_comment))
    identifier = Regex(r'^[a-z_][a-zA-Z0-9_]*')
    string = Choice(r_single_quote_str, r_double_quote_str)
    scope = Ref()
    action = Ref()

    f_create = Sequence(Keyword('create'), '(', identifier, ')')
    f_drop = Sequence(Keyword('drop'), '(', Choice(identifier, thing_id), ')')
    f_set = Sequence(Keyword('drop'), '(', Choice(identifier, thing_id), ')')
    f_rename = Sequence(k_rename_db, '(', Choice(identifier, thing_id), ',', identifier, ')')
    f_thing = Sequence(k_thing, '(', thing_id, ')')
    f_fetch = Sequence(k_fetch, '(', List(identifier, opt=True), ')')
    f_map = Sequence(k_map, '(', List(identifier, mi=1, ma=2), '=>', scope, ')')

    selectors = Sequence(
        Optional(Sequence(
            '[',
            Choice(identifier, array_idx),
            ']'
        )),
        Optional(Sequence('.', action))
    )

    action = Choice(
        f_fetch,
        Sequence('=', Choice(string, scope)),
        Sequence(
            Choice(f_map, identifier),
            selectors,
        ),
    )

    scope = Sequence(
        Choice(f_thing, identifier),
        selectors
    )

    statement = Sequence(
        comment,
        Choice(
            f_create_db,
            f_drop_db,
            f_rename_db,
            scope
        )
    )

    START = Sequence(
        List(statement, delimiter=';', opt=True),
        comment
    )

    @classmethod
    def translate(cls, elem):
        if elem == cls.identifier:
            return 'identifier'
        elif elem == cls.thing_id:
            return 'id'

    def test(self, str):
        print('{} : {}'.format(
            str.strip(), self.parse(str).as_str(self.translate)))


if __name__ == '__main__':
    definition = Definition()

    definition.test('  fetch().  ')

    definition.test('''
        /*
         * Create a database
         */
        databases.create(dbtest);

        /*
         * Drop a database
         */
        databases.dbtest.drop();

        /* Change redundancy */
        config.set_redundancy(3)

        /*
         * Finished!
         */
    ''')

    definition.test(' users.create(iris); ')
    definition.test(' users.iris.drop(); ')

    definition.test(' users.iris.drop(); ')





    definition.test( ' databases.dbtest.drop() ')

    definition.test('  drop_db( dbtest )  ')
    definition.test('  drop_db( 123 )  ')
    {   # RETURN
        '$id': 123,
        '$dropped': True
    }
    {   # WATCHERS $ID 0
        '$id': 0,
        '$arr': {
            'databases': {
                'rmval': [{
                    '$id': 123,
                }]
            }
        }
    }
    # This will also invoke the unwatchall() on all things in the dropped db
    {   # FOR EXAMPLE: WATCHING 123
        '$id': 123,
        '$dropped': True
    }
    definition.test('  databases.create(dbtest);  ')
    [{   # RETURN
        '$id': 0,
        '$array': {
            '$databases': {
                'push': [{
                    '$id': 123,
                    '$path': '.00000000001_',
                    '$name': 'dbtest'
                }]
            }
        }
    }]
    {   # WATCHERS $ID 0
        '$id': 0,
        '$array': {
            '$databases': {
                'push': [{
                    '$id': 123,
                }]
            }
        }
    }
    definition.test('  databases.create(dbtest);  ')