import re
import sys
from pyleri import (
    Grammar,
    Keyword,
    Regex,
    Choice as Choice_,
    Sequence,
    Ref,
    Prio,
    Tokens,
    Repeat,
    List as List_,
    Optional,
    THIS,
)

RE_NAME = r'^[a-zA-Z_][a-zA-Z0-9_]*'


class Choice(Choice_):
    def __init__(self, *args, most_greedy=None, **kwargs):
        if most_greedy is None:
            most_greedy = False
        super().__init__(*args, most_greedy=most_greedy, **kwargs)


class List(List_):
    def __init__(self, *args, opt=None, **kwargs):
        if opt is None:
            opt = True
        super().__init__(*args, opt=opt, **kwargs)


class Definition(Grammar):
    RE_KEYWORDS = re.compile(RE_NAME)

    r_single_quote = Regex(r"(?:'(?:[^']*)')+")
    r_double_quote = Regex(r'(?:"(?:[^"]*)")+')

    t_false = Keyword('false')
    t_nil = Keyword('nil')
    t_true = Keyword('true')
    t_undefined = Keyword('undefined')
    t_int = Regex(r'[-+]?[0-9]+')
    t_float = Regex(r'[-+]?[0-9]*\.?[0-9]+')
    t_string = Choice(r_single_quote, r_double_quote)

    comment = Repeat(Regex(r'(?s)/\\*.*?\\*/'))

    name = Regex(RE_NAME)

    # build-in get functions
    f_blob = Keyword('blob')
    f_filter = Keyword('filter')
    f_get = Keyword('get')
    f_id = Keyword('id')
    f_map = Keyword('map')
    f_thing = Keyword('thing')

    # build-in update functions
    f_create = Keyword('create')
    f_del = Keyword('del')
    f_drop = Keyword('create')
    f_grant = Keyword('grant')
    f_push = Keyword('push')
    f_rename = Keyword('rename')
    f_revoke = Keyword('revoke')
    f_set = Keyword('set')
    f_unset = Keyword('unset')

    primitives = Choice(
        t_false,
        t_nil,
        t_true,
        t_undefined,
        t_int,
        t_float,
        t_string,
    )

    scope = Ref()
    chain = Ref()

    thing = Sequence('{', List(Sequence(name, ':', scope)), '}')
    array = Sequence('[', List(scope), ']')

    iterator = Sequence(List(name, mi=1, ma=2, opt=False), '=>', scope)
    arguments = List(scope)

    function = Sequence(Choice(
        # build-in get functions
        f_blob,     # (int inx_in_blobs) -> raw
        f_filter,   # (iterator) -> [return values where return is true]
        f_get,      #
        f_id,       # () -> int
        f_map,      # (iterator) -> [return values]
        f_thing,    # (int thing_id) -> thing
        # build-in update functions
        f_create,
        f_del,
        f_drop,
        f_grant,
        f_push,
        f_rename,
        f_revoke,
        f_set,
        f_unset,
        # any name
        name,
    ), '(', Choice(
        iterator,
        arguments,
    ), ')')

    cmp_operators = Tokens('< > == != <= >= ~ !~')

    compare = Sequence(
        '(',
        Prio(
            Sequence(scope, Optional(Sequence(cmp_operators, scope))),
            Sequence('(', THIS, ')'),
            Sequence(THIS, '&&', THIS),  # we could add here + - * / etc.
            Sequence(THIS, '||', THIS)
        ),
        ')',
    )

    assignment = Sequence(name, '=', scope)
    index = Repeat(
        Sequence('[', t_int, ']')
    )

    chain = Sequence(
        '.',
        Choice(function, assignment, name),
        index,
        Optional(chain),
    )

    scope = Sequence(
        Choice(
            primitives,
            function,
            assignment,
            name,
            thing,
            array,
            compare
        ),
        index,
        Optional(chain),
    )

    START = Sequence(
        comment,
        List(scope, delimiter=Sequence(';', comment)),
    )

    @classmethod
    def translate(cls, elem):
        if elem == cls.name:
            return 'name'

    def test(self, str):
        print('{} : {}'.format(
            str.strip(), self.parse(str).as_str(self.translate)))


if __name__ == '__main__':
    definition = Definition()
    definition.test('users.find(user => (user.id == 1)).labels.filter(label => (label.id().i == 1))')
    definition.test('users.find(user => (user.id == 1)).labels.filter(label => (label.id().i == 1))')
    # exit(0)
    definition.test('users.create("iris");grant(users.iris,FULL)')
    definition.test('labels.map(label => label.id()')
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
        config.redundancy = 3[0][0];

        types().create(User, {
            name: str().required(),
            age: int().required(),
            owner: User.required(),
            schools: School.required(),
            scores: thing,
            other: int
        });

        type().add(users, User.isRequired);

        /*
         * Finished!
         */
    ''')

    definition.test(' users.create(iris); ')
    definition.test(' users.iris.drop(); ')

    definition.test(' users.iris.drop(); ')

    definition.test(' databases.dbtest.drop() ')

    definition.test('  drop_db( dbtest )  ')
    definition.test('  {a: "123"}.b = 4  ')
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

    {
        '$ev': 0,
        '$id': 4,
        '$jobs': [
            {'assign': {'age', 5}},
            {'del': 'age'},
            {'set': {'name': 'iris'}},
            {'set': {'image': '<bin_data>'}},
            {'unset': 'name'},
            {'push': {'people': [{'$id': 123}]}}
        ]
    }

    {
        'ev': 0,
        'id': 4,
        'set': ['age', 5]
    }

    c, h = definition.export_c(target='langdef', headerf='<langdef/langdef.h>')
    with open('../src/langdef/langdef.c', 'w') as cfile:
        cfile.write(c)

    with open('../inc/langdef/langdef.h', 'w') as hfile:
        hfile.write(h)

    print('Finished export to c')
