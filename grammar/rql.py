from pyleri import Choice
from pyleri import Grammar
from pyleri import Keyword
from pyleri import List
from pyleri import Optional
from pyleri import Prio
from pyleri import Regex
from pyleri import Repeat
from pyleri import Sequence
from pyleri import THIS
from pyleri import Token
from pyleri import Tokens


class RqlLang(Grammar):

    r_uint = Regex('[0-9]+')
    r_int = Regex('[-+]?[0-9]+')
    r_float = Regex('[-+]?[0-9]*\.?[0-9]+')
    r_str = Regex('(?:"(?:[^"]*)")+')

    r_name = Regex('[A-Za-z][A-Za-z0-9_]*')

    elem_id = Repeat(r_uint, 1, 1)
    kind = Repeat(r_name, 1, 1)
    prop = Repeat(r_name, 1, 1)
    mark = Repeat(r_name, 1, 1)
    compare_opr = Tokens('< > == != <= >= ~ !~')

    t_and = Token('&&')
    t_or = Token('||')

    filter = Sequence(
        Token('{'),
        Optional(
            Prio(
                Sequence(prop, compare_opr, Choice(
                    r_uint,
                    r_int,
                    r_float,
                    r_str,
                    most_greedy=False
                )),
                Sequence('(', THIS, ')'),
                Sequence(THIS, t_and, THIS),
                Sequence(THIS, t_or, THIS)
            )
        ),
        Token('}'))

    selector = Sequence(
        Optional(Token('!')),
        Choice(
                elem_id,
                kind,
                most_greedy=False
        ),
        Optional(
            Choice(
                Token('*'),
                Sequence(
                    Token('('),
                    List(
                        Sequence(
                            prop,
                            Optional(
                                Choice(
                                    Sequence(
                                        Token('#'),
                                        prop
                                    ),
                                    Sequence(
                                        Token('.'),
                                        mark
                                    ),
                                    most_greedy=False
                                )
                            ),
                            Optional(filter)
                        ),
                        mi=1
                    ),
                    Token(')')
                ),
                most_greedy=False
            )
        )
    )

    START = List(
        selector,
        mi=1
    )

if __name__ == '__main__':
    rql = RqlLang()

    pr = rql.parse('!ID(hosts),Host(name, alerts#open{status=="open"}, alerts#open{status=="closed"}, names{}),Alert(messages),Message*')
    print(pr.tree)
