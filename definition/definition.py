import re
from pyleri import (
    Grammar,
    Keyword,
    Regex,
    Choice,
    Sequence,
    Ref,
    Prio,
    Repeat,
    List,
    Optional,
    THIS,
)



class Definition(Grammar):
    RE_KEYWORDS = re.compile(r'^[$a-z][a-z_]*')


    r_single_quote_str = Regex(r"(?:'(?:[^']*)')+")
    r_double_quote_str = Regex(r'(?:"(?:[^"]*)")+')

    k_create_db = Keyword('create_db')
    k_drop_db = Keyword('drop_db')
    k_rename_db = Keyword('rename_db')
    k_fetch = Keyword('fetch')
    k_thing = Keyword('thing')
    k_map = Keyword('map')

    thing_id = Regex(r'[0-9]+')
    identifier = Regex(r'^[a-z_][a-zA-Z0-9_]*')
    string = Choice(r_single_quote_str, r_double_quote_str, most_greedy=False)
    scope = Ref()


    f_create_db = Sequence(k_create_db, '(', identifier, ')')
    f_drop_db = Sequence(k_drop_db, '(', identifier, ')')
    f_rename_db = Sequence(k_rename_db, '(', identifier, ',', identifier, ')')
    f_thing = Sequence(k_thing, '(', thing_id, ')')
    f_fetch = Sequence(k_fetch, '(', List(identifier, opt=True), ')')
    f_map = Sequence(k_map, '(', List(identifier, mi=1, ma=2), '=>', scope, ')')

    action = Prio(
        f_fetch,
        Sequence(
            Choice(f_map, identifier, most_greedy=False),
            Optional(Sequence('.', THIS))
        )
    )

    scope = Choice(
        Sequence(
            Choice(f_thing, identifier, most_greedy=False),
            Optional(Sequence('.', action))
        ),
        Sequence(identifier, '=', Choice(string, scope, most_greedy=False))
    )

    START = Choice(
        f_create_db,
        f_drop_db,
        f_rename_db,
        scope,
        most_greedy=False,
    )

    @classmethod
    def translate(cls, elem):
        if elem == cls.identifier:
            return 'identifier'
        elif elem == cls.thing_id:
            return 'id'


if __name__ == '__main__':
    definition = Definition()
    print(definition.parse('hoi.hoi.hoi.map(a, b => thing(10))').as_str(definition.translate))