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

# names have a max length of 255 characters
RE_NAME = r'^[A-Za-z_][0-9A-Za-z_]{0,254}(?![0-9A-Za-z_])'


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

    x_array = Token('[')
    x_assign = Tokens('= += -= *= /= %= &= ^= |=')
    x_block = Token('{')
    x_chain = Tokens('.. .')
    x_closure = Token('|')
    x_function = Token('(')
    x_index = Token('[')
    x_parenthesis = Token('(')
    x_preopr = Regex(r'(\s*!|\s*[\-+](?=[^0-9]))*')
    x_ternary = Token('?')
    x_thing = Token('{')
    x_template = Token('`')

    template = Sequence(
        x_template,
        Repeat(Choice(
            Regex(r"([^`{}]|``|{{|}})+"),
            Sequence('{', THIS, '}')
        )),
        x_template
    )

    t_false = Keyword('false')
    t_float = Regex(
        r'[-+]?(inf|nan|[0-9]*\.[0-9]+(e[+-][0-9]+)?)'
        r'(?![0-9A-Za-z_\.])')
    t_int = Regex(
        r'[-+]?((0b[01]+)|(0o[0-8]+)|(0x[0-9a-fA-F]+)|([0-9]+))'
        r'(?![0-9A-Za-z_\.])')

    t_nil = Keyword('nil')
    t_regex = Regex(r'/((?:.(?!(?<![\\])/))*.?)/[a-z]*')
    t_string = Regex(r"""(((?:'(?:[^']*)')+)|((?:"(?:[^"]*)")+))""")
    t_true = Keyword('true')

    # It would be nice if the leri family had support for advanced white space.
    # If so, the comments could be set as white space instead.
    comments = Repeat(Choice(
        Regex(r'(?s)//.*?(\r?\n|$)'),  # Single line comment
        Regex(r'(?s)/\*.*?\*/'),  # Block comment
    ))

    name = Regex(RE_NAME)
    var = Regex(RE_NAME)

    chain = Ref()

    closure = Sequence(x_closure, List(var), '|', THIS)

    thing = Sequence(x_thing, List(Sequence(name, ':', Optional(THIS))), '}')
    array = Sequence(x_array, List(THIS), ']')
    function = Sequence(x_function, List(THIS), ')')
    instance = Repeat(thing, mi=1, ma=1)  # will be exported as `cleri_dup_t`
    enum_ = Sequence(x_thing, Choice(name, closure), '}')

    opr0_mul_div_mod = Tokens('* / %')
    opr1_add_sub = Tokens('+ -')
    opr2_bitwise_and = Tokens('&')
    opr3_bitwise_xor = Tokens('^')
    opr4_bitwise_or = Tokens('|')
    opr5_compare = Tokens('< > == != <= >=')
    opr6_cmp_and = Token('&&')
    opr7_cmp_or = Token('||')
    opr8_ternary = Sequence(x_ternary, THIS, ':')

    operations = Sequence(THIS, Choice(
        # make sure `ternary`, `and` and `or` is on top so we can stop
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

    assign = Sequence(x_assign, THIS)

    name_opt_more = Sequence(name, Optional(Choice(function, assign)))
    var_opt_more = Sequence(
        var,
        Optional(Choice(function, assign, instance, enum_)))

    # note: slice is also used for a simple index
    slice = List(Optional(THIS), delimiter=':', ma=3, opt=False)

    index = Repeat(Sequence(
        x_index, slice, ']',
        Optional(Sequence(x_assign, THIS))
    ))

    chain = Sequence(
        x_chain,
        name_opt_more,
        index,
        Optional(chain),
    )

    end_statement = \
        Regex(r'((;|((?s)\/\/.*?(\r?\n|$))|((?s)\/\*.*?\*\/))\s*)*')

    block = Sequence(
        x_block,
        comments,
        List(THIS, delimiter=end_statement, mi=1),
        '}')

    parenthesis = Sequence(x_parenthesis, THIS, ')')

    k_if = Keyword('if')
    k_else = Keyword('else')
    k_return = Keyword('return')
    k_for = Keyword('for')
    k_in = Keyword('in')
    k_continue = Keyword('continue')
    k_break = Keyword('break')

    if_statement = Sequence(
        k_if,
        '(',
        THIS,
        ')',
        THIS,
        Optional(Sequence(k_else, THIS)))

    return_statement = Sequence(
        k_return,
        List(THIS, mi=1, ma=3, opt=False))

    for_statement = Sequence(
        k_for,
        '(',
        List(var, mi=1, opt=False),
        k_in,
        THIS,
        ')',
        THIS)

    expression = Sequence(
        x_preopr,
        Choice(
            chain,
            # start immutable values
            t_false,
            t_nil,
            t_true,
            t_float,
            t_int,
            t_string,
            t_regex,
            # end immutable values
            template,
            var_opt_more,
            thing,
            array,
            parenthesis,
        ),
        index,
        Optional(chain),
    )

    statement = Prio(
        k_continue,
        k_break,
        Choice(
            if_statement,
            return_statement,
            for_statement,
            closure,
            expression,
            block,
        ),
        operations)
    statements = List(statement, delimiter=end_statement)

    START = Sequence(comments, statements)


if __name__ == '__main__':
    langdef = LangDef()
    res = langdef.parse(r'''x = /./;''')
    print(res.is_valid)

    res = langdef.parse(r'''/./;''')
    print(res.is_valid)

    res = langdef.parse(r'''|x|...)''')
    print(res.is_valid)

    res = langdef.parse(r'''a = 5;''')
    print(res.is_valid)

    res = langdef.parse(r"""//ti
        if (x > 5) {
            return {x: x}, 5;
        }
    """)
    print(res.is_valid)

    res = langdef.parse(r"""//ti
        for (x in range(3)) {
            if (x < 2) continue;
            return {x: x}, 5;
        }
    """)
    print(res.is_valid)

    # exit(0)

    c, h = langdef.export_c(target='langdef', headerf='<langdef/langdef.h>')
    with open('../src/langdef/langdef.c', 'w') as cfile:
        cfile.write(c)

    with open('../inc/langdef/langdef.h', 'w') as hfile:
        hfile.write(h)

    print('Finished export to c')
