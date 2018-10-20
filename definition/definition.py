import re
import sys
sys.path.insert(0, '../../pyleri')
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

RE_IDENTIFIER = r'^[a-zA-Z_][a-zA-Z0-9_]*'

class Choice(Choice_):
    def __init__(self, *args, most_greedy=None, **kwargs):
        if most_greedy is None:
            most_greedy = False
        super().__init__(*args, most_greedy=most_greedy, **kwargs)


class Definition(Grammar):
    RE_KEYWORDS = re.compile(RE_IDENTIFIER)

    r_single_quote_str = Regex(r"(?:'(?:[^']*)')+")
    r_double_quote_str = Regex(r'(?:"(?:[^"]*)")+')

    r_int = Regex(r'[-+]?[0-9]+')
    r_float = Regex(r'[-+]?[0-9]*\.?[0-9]+')

    r_comment = Regex(r'(?s)/\\*.*?\\*/')

    array_idx = Regex(r'[-+]?[0-9]+')
    thing_id = Regex(r'[0-9]+')

    comment = Optional(Repeat(r_comment))
    identifier = Regex(RE_IDENTIFIER)
    string = Choice(r_single_quote_str, r_double_quote_str)
    scope = Ref()
    chain = Ref()

    primitives = Choice(
        Keyword('nil'),
        Keyword('false'),
        Keyword('true'),
        r_int,
        r_float,
        string
    )

    auth_flags = List(Choice(
        Keyword('FULL'),
        Keyword('ACCCESS'),
        Keyword('READ'),
        Keyword('MODIFY'),
        Regex(r'[0-9]+')), delimiter='|'
    )

    f_blob = Sequence(Keyword('blob'), '(', Optional(array_idx), ')')
    f_thing = Sequence(Keyword('thing'), '(', Optional(thing_id), ')')
    f_create = Sequence(Keyword('create'), '(', List(identifier, opt=True), ')')
    f_grant = Sequence(Keyword('grant'), '(', auth_flags, ')')
    f_revoke = Sequence(Keyword('revoke'), '(', auth_flags, ')')
    f_drop = Sequence(Keyword('drop'), '(', ')')
    f_delete = Sequence(Keyword('delete'), '(', ')')
    f_rename = Sequence(Keyword('rename'), '(', identifier, ')')
    f_fetch = Sequence(Keyword('fetch'), '(', List(identifier, opt=True), ')')
    f_map = Sequence(Keyword('map'), '(', List(identifier, mi=1, ma=2), '=>', scope, ')')

    action = Choice(
        Sequence('.', chain),
        Sequence('=', Choice(primitives, f_blob, scope)),
    )

    chain = Sequence(
        Choice(
            f_create,
            f_drop,
            f_rename,
            f_fetch,
            f_grant,
            f_revoke,
            f_delete,
            f_map,
            identifier
        ),
        Optional(Sequence(
            '[',
            Choice(identifier, array_idx),
            ']'
        )),
        Optional(action)
    )

    scope = Sequence(
        Choice(f_thing, identifier),
        Optional(Sequence(
            '[',
            Choice(identifier, array_idx),
            ']'
        )),
        Optional(action)
    )

    statement = Sequence(comment, scope)

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

    definition.test('users.create(iris);users.iris.grant(FULL)')
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
        config.redundancy = 3;

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

    c, h = definition.export_c(target='definition', headerf='<definition.h>')
    with open('../src/definition.c', 'w') as cfile:
        cfile.write(c)

    with open('../inc/definition.h', 'w') as hfile:
        hfile.write(h)

