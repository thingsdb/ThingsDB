/* jshint newcap: false */

/*
 * This grammar is generated using the Grammar.export_js() method and
 * should be used with the jsleri JavaScript module.
 *
 * Source class: RqlLang
 * Created at: 2017-09-29 22:49:23
 */

'use strict';

(function (
            THIS,
            Rule,
            Repeat,
            List,
            Optional,
            Choice,
            Regex,
            Token,
            Sequence,
            Prio,
            Grammar,
            Tokens
        ) {
    var r_uint = Regex('^[0-9]+');
    var r_int = Regex('^[-+]?[0-9]+');
    var r_float = Regex('^[-+]?[0-9]*\\.?[0-9]+');
    var r_str = Regex('^(?:"(?:[^"]*)")+');
    var r_name = Regex('^[A-Za-z][A-Za-z0-9_]*');
    var elem_id = Repeat(r_uint, 1, 1);
    var kind = Repeat(r_name, 1, 1);
    var prop = Repeat(r_name, 1, 1);
    var mark = Repeat(r_name, 1, 1);
    var compare_opr = Tokens('== != <= >= !~ < > ~');
    var t_and = Token('&&');
    var t_or = Token('||');
    var filter = Sequence(
        Token('{'),
        Optional(Prio(
            Sequence(
                prop,
                compare_opr,
                Choice(
                    r_uint,
                    r_int,
                    r_float,
                    r_str
                )
            ),
            Sequence(
                Token('('),
                THIS,
                Token(')')
            ),
            Sequence(
                THIS,
                t_and,
                THIS
            ),
            Sequence(
                THIS,
                t_or,
                THIS
            )
        )),
        Token('}')
    );
    var selector = Sequence(
        Optional(Token('!')),
        Choice(
            elem_id,
            kind
        ),
        Optional(Choice(
            Token('*'),
            Sequence(
                Token('('),
                List(Sequence(
                    prop,
                    Optional(Choice(
                        Sequence(
                            Token('#'),
                            prop
                        ),
                        Sequence(
                            Token('.'),
                            mark
                        )
                    )),
                    Optional(filter)
                ), Token(','), 1, undefined, false),
                Token(')')
            )
        ))
    );
    var START = List(selector, Token(','), 1, undefined, false);

    window.RqlLang = Grammar(START, '^\\w+');

})(
    window.jsleri.THIS,
    window.jsleri.Rule,
    window.jsleri.Repeat,
    window.jsleri.List,
    window.jsleri.Optional,
    window.jsleri.Choice,
    window.jsleri.Regex,
    window.jsleri.Token,
    window.jsleri.Sequence,
    window.jsleri.Prio,
    window.jsleri.Grammar,
    window.jsleri.Tokens
);
