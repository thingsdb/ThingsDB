"""Source ThingsDB language definition file.
"""
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
ASSIGN_TOKENS = Tokens('= += -= *= /= %= &= ^= |=')


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


class LangDef(Grammar):
    RE_KEYWORDS = re.compile(RE_NAME)

    r_single_quote = Regex(r"(?:'(?:[^']*)')+")
    r_double_quote = Regex(r'(?:"(?:[^"]*)")+')

    thing_by_id = Regex(r'#[0-9]+')

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
    comments = Repeat(Choice(
        Regex(r'(?s)//.*?\r?\n'),  # Single line comment
        Regex(r'(?s)/\\*.*?\\*/'),  # Block comment
    ))

    name = Regex(RE_NAME)
    var = Regex(RE_NAME)

    chain = Ref()

    t_closure = Sequence('|', List(var), '|', THIS)

    thing = Sequence('{', List(Sequence(name, ':', THIS)), '}')
    array = Sequence('[', List(THIS), ']')
    function = Sequence('(', List(THIS), ')')

    immutable = Choice(
        t_false,
        t_nil,
        t_true,
        t_float,
        t_int,
        t_string,
        t_regex,
        t_closure,
    )

    opr0_mul_div_mod = Tokens('* / % //')
    opr1_add_sub = Tokens('+ -')
    opr2_bitwise_and = Tokens('&')
    opr3_bitwise_xor = Tokens('^')
    opr4_bitwise_or = Tokens('|')
    opr5_compare = Tokens('< > == != <= >=')
    opr6_cmp_and = Token('&&')
    opr7_cmp_or = Token('||')
    opr8_ternary = Sequence('?', THIS, ':')

    operations = Sequence(THIS, Choice(
        # make sure `and` and `or` is on top so we can stop
        # at the first match
        opr8_ternary,
        opr7_cmp_or,
        opr6_cmp_and,
        opr5_compare,
        opr4_bitwise_or,
        opr3_bitwise_xor,
        opr2_bitwise_and,
        opr1_add_sub,
        opr0_mul_div_mod,
    ), THIS)

    assign = Sequence(ASSIGN_TOKENS, THIS)

    name_opt_func_assign = Sequence(name, Optional(Choice(function, assign)))
    var_opt_func_assign = Sequence(var, Optional(Choice(function, assign)))

    # note: slice is also used for a simple index
    slice = List(Optional(THIS), delimiter=':', ma=3, opt=False)

    index = Repeat(Sequence(
        '[', slice, ']',
        Optional(Sequence(ASSIGN_TOKENS, THIS))
    ))

    chain = Sequence(
        '.',
        name_opt_func_assign,
        index,
        Optional(chain),
    )

    block = Sequence(
        '{',
        comments,
        List(THIS, delimiter=Sequence(';', comments), mi=1),
        '}')

    parenthesis = Sequence('(', THIS, ')')

    expression = Sequence(
        o_not,
        Choice(
            chain,
            thing_by_id,
            immutable,
            var_opt_func_assign,
            thing,
            array,
            block,
            parenthesis,
        ),
        index,
        Optional(chain),
    )

    statement = Prio(expression, operations)
    statements = List(statement, delimiter=Sequence(';', comments))

    START = Sequence(comments, statements)

    @classmethod
    def translate(cls, elem):
        if elem == cls.name:
            return 'name'

    def test(self, str):
        print('{} : {}'.format(
            str.strip(), self.parse(str).as_str(self.translate)))


if __name__ == '__main__':
    langdef = LangDef()

    langdef.test(r'''
        (1 && 2) - 2;
        true ? 4 : 5;
        !true ? 4 : 5;
        !(true ? 4 : 5);
        .a = 1;
        .a.b = 2;
        !!!.a[0][1].c['bla'];
        .map(|x, y| x == y);
    ''')
    # exit(0)

    c, h = langdef.export_c(target='langdef', headerf='<langdef/langdef.h>')
    with open('../src/langdef/langdef.c', 'w') as cfile:
        cfile.write(c)

    with open('../inc/langdef/langdef.h', 'w') as hfile:
        hfile.write(h)

    print('Finished export to c')
