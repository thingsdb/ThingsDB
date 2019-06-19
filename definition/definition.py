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

    # build-in get functions
    f_assert = Keyword('assert')
    f_blob = Keyword('blob')
    f_bool = Keyword('bool')
    f_endswith = Keyword('endswith')
    f_filter = Keyword('filter')
    f_findindex = Keyword('findindex')
    f_find = Keyword('find')
    f_has = Keyword('has')
    f_hasprop = Keyword('hasprop')
    f_id = Keyword('id')
    f_indexof = Keyword('indexof')
    f_int = Keyword('int')
    f_isarray = Keyword('isarray')
    f_isascii = Keyword('isascii')
    f_isbool = Keyword('isbool')
    f_isfloat = Keyword('isfloat')
    f_isinf = Keyword('isinf')
    f_isint = Keyword('isint')
    f_islist = Keyword('islist')
    f_isnan = Keyword('isnan')
    f_israw = Keyword('israw')
    f_isstr = Keyword('isstr')  # alias for isutf8 (if isutf8, then is isascii)
    f_istuple = Keyword('istuple')
    f_isutf8 = Keyword('isutf8')
    f_len = Keyword('len')
    f_lower = Keyword('lower')
    f_map = Keyword('map')
    f_now = Keyword('now')
    f_refs = Keyword('refs')
    f_ret = Keyword('ret')
    f_set = Keyword('set')
    f_startswith = Keyword('startswith')
    f_str = Keyword('str')
    f_t = Keyword('t')
    f_test = Keyword('test')
    f_try = Keyword('try')
    f_upper = Keyword('upper')

    # build-in update functions
    f_add = Keyword('add')
    f_del = Keyword('del')
    f_pop = Keyword('pop')
    f_push = Keyword('push')
    f_remove = Keyword('remove')
    f_rename = Keyword('rename')
    f_splice = Keyword('splice')

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

    function = Sequence(Choice(
        # build-in functions
        f_assert,       # (statement, [msg], [code])
        f_blob,         # (int inx_in_blobs) -> raw
        f_bool,         # () -> bool
        f_endswith,     # (str) -> bool
        f_filter,       # (closure) -> [return values where return is true]
        f_findindex,    # (closure) -> return the index of the first value..
        f_find,         # (closure, alt) -> return first value where true...
        f_has,          # (thing) -> bool
        f_hasprop,      # (str) -> bool
        f_id,           # () -> int
        f_indexof,      # (v) -> int or nil
        f_int,          # (x) -> int
        f_isarray,      # (x) -> bool
        f_isascii,
        f_isbool,
        f_isfloat,
        f_isinf,        # (float) -> bool
        f_isint,
        f_islist,       # (x) -> bool
        f_isnan,        # (float) -> bool
        f_israw,
        f_isstr,        # alias for isutf8 (if isutf8, then is isascii)
        f_istuple,
        f_isutf8,
        f_len,          # () -> int
        f_lower,        # () -> str
        f_map,          # (closure) -> [return values]
        f_now,          # () -> timestamp as double seconds.nanoseconds
        f_refs,         # () -> reference counter
        f_ret,          # () -> nil
        f_set,          # ([]) -> new set
        f_startswith,   # (str) -> bool
        f_str,          # (x) -> raw
        f_t,            # (int thing_id) -> thing
        f_test,         # (regex) -> bool
        f_try,
        f_upper,        # () -> str
        # build-in `event` functions
        f_add,
        f_del,
        f_pop,
        f_push,
        f_remove,       # (closue/thing, alt) -> remove and return first value...
        f_rename,       #
        f_splice,       #
        # any name
        name,           # used for `root` functions
    ), '(', List(scope), ')')

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
        Optional(Sequence('?', scope, ':', scope)),  # conditional support?
    )

    assignment = Sequence(name, Tokens(ASSIGN_TOKENS), scope)
    tmp_assign = Sequence(tmp, Tokens(ASSIGN_TOKENS), scope)

    index = Repeat(
        Sequence('[', scope, ']')
    )       # we skip index in query investigate (in case we want to use scope)

    chain = Sequence(
        '.',
        Choice(function, assignment, name),
        index,
        Optional(chain),
    )

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

    definition.test(r'''

        (true).refs();

        ''')
    # exit(0)

    c, h = definition.export_c(target='langdef', headerf='<langdef/langdef.h>')
    with open('../src/langdef/langdef.c', 'w') as cfile:
        cfile.write(c)

    with open('../inc/langdef/langdef.h', 'w') as hfile:
        hfile.write(h)

    print('Finished export to c')
