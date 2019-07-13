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
    Token,
    Tokens,
    Repeat,
    List as List_,
    Optional,
    THIS,
)


RE_NAME = r'^[A-Za-z_][0-9A-Za-z_]*'
RE_TMP = r'^\$[A-Za-z_][0-9A-Za-z_]*'
ASSIGN_TOKENS = '= += -= *= /= %= &= ^= |='


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
    t_float = Regex(
        r'[-+]?((inf|nan)([^0-9A-Za-z_]|$)|[0-9]*\.[0-9]+(e[+-][0-9]+)?)')
    t_int = Regex(
        r'[-+]?((0b[01]+)|(0o[0-8]+)|(0x[0-9a-fA-F]+)|([0-9]+))')
    t_nil = Keyword('nil')
    t_regex = Regex('(/[^/\\\\]*(?:\\\\.[^/\\\\]*)*/i?)')
    t_string = Choice(r_single_quote, r_double_quote)
    t_true = Keyword('true')

    o_not = Repeat(Token('!'))
    comment = Repeat(Regex(r'(?s)/\\*.*?\\*/'))

    name = Regex(RE_NAME)
    tmp = Regex(RE_TMP)

    primitives = Choice(
        t_false,
        t_nil,
        t_true,
        t_float,
        t_int,
        t_string,
        t_regex,
    )

    scope = Ref()
    chain = Ref()

    thing = Sequence('{', List(Sequence(name, ':', scope)), '}')
    array = Sequence('[', List(scope), ']')

    closure = Sequence('|', List(name), '|', scope)
    function = Sequence(name, '(', List(scope), ')')

    opr0_mul_div_mod = Tokens('* / % //')
    opr1_add_sub = Tokens('+ -')
    opr2_bitwise_and = Tokens('&')
    opr3_bitwise_xor = Tokens('^')
    opr4_bitwise_or = Tokens('|')
    opr5_compare = Tokens('< > == != <= >=')
    opr6_cmp_and = Token('&&')
    opr7_cmp_or = Token('||')

    operations = Sequence(
        '(',
        Prio(
            scope,
            Sequence(THIS, Choice(
                opr0_mul_div_mod,
                opr1_add_sub,
                opr2_bitwise_and,
                opr3_bitwise_xor,
                opr4_bitwise_or,
                opr5_compare,
                opr6_cmp_and,
                opr7_cmp_or,
                most_greedy=True,
            ), THIS)
        ),
        ')',
        Optional(Sequence('?', scope, ':', scope)),
    )

    assignment = Sequence(name, Tokens(ASSIGN_TOKENS), scope)
    tmp_assign = Sequence(tmp, Tokens(ASSIGN_TOKENS), scope)

    index = Repeat(
        Sequence('[', scope, ']')
    )

    chain = Sequence(
        '.',
        Choice(function, assignment, name),
        index,
        Optional(chain),
    )

    # maybe we can optimize since for example `name` is captured tree times
    # it might be even better to optimize libcleri for handling these cases
    scope = Sequence(
        o_not,
        Choice(
            primitives,
            function,
            assignment,
            tmp_assign,
            name,
            closure,
            tmp,
            thing,
            array,
            operations,
        ),
        index,
        Optional(chain),
    )

    statements = List(scope, delimiter=Sequence(';', comment))

    deep = Sequence('=>', t_int)

    START = Sequence(comment, statements, Optional(deep))

    @classmethod
    def translate(cls, elem):
        if elem == cls.name:
            return 'name'

    def test(self, str):
        print('{} : {}'.format(
            str.strip(), self.parse(str).as_str(self.translate)))


if __name__ == '__main__':
    definition = Definition()

    definition.test(r'''
        /*
         * Test query parsing block
         */

    ''')
    # exit(0)

    c, h = definition.export_c(target='langdef', headerf='<langdef/langdef.h>')
    with open('../src/langdef/langdef.c', 'w') as cfile:
        cfile.write(c)

    with open('../inc/langdef/langdef.h', 'w') as hfile:
        hfile.write(h)

    print('Finished export to c')
