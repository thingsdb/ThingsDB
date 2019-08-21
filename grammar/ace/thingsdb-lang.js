define(function(require, exports, module) {
    "use strict";

    var oop = require("../lib/oop");
    var DocCommentHighlightRules = require("./doc_comment_highlight_rules").DocCommentHighlightRules;
    var TextHighlightRules = require("./text_highlight_rules").TextHighlightRules;

    var rootFunctions = exports.rootFunctions = "\\b(new_user|new_procedure|new_node|counters|refs|now)(?=(\\s*\\())";

    var dotFunctions = exports.dotFunctions = "\\.\\s*(map|filter|id|len)(?=(\\s*\\())";

    var thingsdbHighlightRules = function() {

        var keywordControls = (
            "break|case|continue|default|do|else|for|goto|if|_Pragma|" +
            "return|switch|while|catch|operator|try|throw|using"
        );

        var storageType = (
            "asm|__asm__|auto|bool|_Bool|char|_Complex|double|enum|float|" +
            "_Imaginary|int|long|short|signed|struct|typedef|union|unsigned|void|" +
            "class|wchar_t|template|char16_t|char32_t"
        );

        var storageModifiers = (
            "const|extern|register|restrict|static|volatile|inline|private|" +
            "protected|public|friend|explicit|virtual|export|mutable|typename|" +
            "constexpr|new|delete|alignas|alignof|decltype|noexcept|thread_local"
        );

        var keywordOperators = (
            "and|and_eq|bitand|bitor|compl|not|not_eq|or|or_eq|typeid|xor|xor_eq|" +
            "const_cast|dynamic_cast|reinterpret_cast|static_cast|sizeof|namespace"
        );

        var builtinConstants = (
            "true|false|nil"
        );

        var keywordMapper = this.$keywords = this.createKeywordMapper({
            "keyword.control" : keywordControls,
            "storage.type" : storageType,
            "storage.modifier" : storageModifiers,
            "keyword.operator" : keywordOperators,
            "constant.language": builtinConstants
        }, "identifier");

        // regexp must not have capturing parentheses. Use (?:) instead.
        // regexps are ordered -> the first match is used

        this.$rules = {
            "start" : [
                {
                    token : "comment",
                    regex : "//.*$",
                    next : "start"
                },
                DocCommentHighlightRules.getStartRule("doc-start"),
                {
                    token : "comment", // multi line comment
                    regex : "\\/\\*",
                    next : "comment"
                }, {
                    token : "string.double.start",
                    regex : '"',
                    next: [{
                        token: "string.double.end",
                        regex: '"|$',
                        next: "start"
                    }, {
                        defaultToken: "string.double"
                    }]
                }, {
                    token : "string.single.start",
                    regex : '\'',
                    next: [{
                        token: "string.single.end",
                        regex: '\'|$',
                        next: "start"
                    }, {
                        defaultToken: "string.single"
                    }]
                }, {
                    token : "constant.numeric", // hex
                    regex : "0[xX][0-9a-fA-F]+(L|l|UL|ul|u|U|F|f|ll|LL|ull|ULL)?\\b"
                }, {
                    token : "constant.numeric", // float
                    regex : "[+-]?\\d+(?:(?:\\.\\d*)?(?:[eE][+-]?\\d+)?)?(L|l|UL|ul|u|U|F|f|ll|LL|ull|ULL)?\\b"
                }, {
                    token : "keyword", // pre-compiler directives
                    regex : "#\\s*(?:scope|use)\\b",
                    next  : "directive"
                }, {
                    token : "keyword", // special case pre-compiler directive
                    regex : "#\\s*(?:endif|if|ifdef|else|elif|ifndef)\\b"
                },  {
                    token : "variable.language",
                    regex : "(name|age|time)(?=(\\s*:))"
                },  {
                    token : "support.function.root.thingsdb",
                    regex : rootFunctions
                }, {
                    token : "support.function.dot.thingsdb",
                    regex : dotFunctions
                }, {
                    token : "variable.language",
                    regex : "\\.\\s*([A-Za-z_][A-Za-z_0-9]*)(?!(\\s*\\())\\b"
                }, {
                    token : keywordMapper,
                    regex : "[a-zA-Z_$][a-zA-Z0-9_$]*"
                }, {
                    token : "keyword.operator",
                    regex : /&&|\|\||\?:|[*%\/+\-&\^|~!<>=]=?/,
                    next  : "start"
                }, {
                  token : "punctuation.operator",
                  regex : "\\?|\\:|\\,|\\;|\\."
                }, {
                    token : "paren.lparen",
                    regex : "[[({]"
                }, {
                    token : "paren.rparen",
                    regex : "[\\])}]"
                }, {
                    token : "text",
                    regex : "\\s+"
                }
            ],
            "comment" : [
                {
                    token : "comment", // closing comment
                    regex : "\\*\\/",
                    next : "start"
                }, {
                    defaultToken : "comment"
                }
            ],
            "singleLineComment" : [
                {
                    token : "comment",
                    regex : /\\$/,
                    next : "singleLineComment"
                }, {
                    token : "comment",
                    regex : /$/,
                    next : "start"
                }, {
                    defaultToken: "comment"
                }
            ],
            "directive" : [
                {
                    token : "constant.other.multiline",
                    regex : /\\/
                },
                {
                    token : "constant.other.multiline",
                    regex : /.*\\/
                },
                {
                    token : "constant.other",
                    regex : "\\s*<.+?>",
                    next : "start"
                },
                {
                    token : "constant.other", // single line
                    regex : '\\s*["](?:(?:\\\\.)|(?:[^"\\\\]))*?["]',
                    next : "start"
                },
                {
                    token : "constant.other", // single line
                    regex : "\\s*['](?:(?:\\\\.)|(?:[^'\\\\]))*?[']",
                    next : "start"
                },
                // "\" implies multiline, while "/" implies comment
                {
                    token : "constant.other",
                    regex : /[^\\\/]+/,
                    next : "start"
                }
            ]
        };

        this.embedRules(DocCommentHighlightRules, "doc-",
            [ DocCommentHighlightRules.getEndRule("start") ]);
        this.normalizeRules();
    };

    oop.inherits(thingsdbHighlightRules, TextHighlightRules);

    exports.thingsdbHighlightRules = thingsdbHighlightRules;
    });
