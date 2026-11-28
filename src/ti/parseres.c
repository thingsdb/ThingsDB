/*
 * ti/parseres.c
 */
#include <langdef/translate.h>
#include <ti/parseres.h>
#include <util/logger.h>

/* improve error message, pr #468 */
static int parseres__test_ambiguous_ne(cleri_parse_t *pr, ex_t *e)
{
    register const char *str = pr->str + pr->pos;
    if (pr->pos >= 4 &&
        *str == '=' &&
        *(--str) == '!' &&
        *(--str) == ')')
    {
        size_t line, pos;
        cleri_parse_expect_start(pr);
        cleri_parse_line_pos(pr, &line, &pos);
        ex_set(e, EX_SYNTAX_ERROR,
            "error at line %zu, position %zu, ambiguous operator `!=`, "
            "add a space between `)` and `!=` to resolve",
            line, pos-1);
        return e->nr;
    }
    return 0;
}

int ti_parseres_syntax_err(cleri_parse_t *pr, ex_t *e)
{
    if (parseres__test_ambiguous_ne(pr, e))
        return e->nr;

    int i = cleri_parse_strn(e->msg, EX_MAX_SZ, pr, &langdef_translate);
    assert_log(i<EX_MAX_SZ, "expecting >= max size %d>=%d", i, EX_MAX_SZ);

    e->msg[EX_MAX_SZ] = '\0';

    log_warning(
            "invalid syntax: `%s` (%s)", pr->str, e->msg);

    /* we will certainly will not hit the max size, but just to be safe */
    e->n = i < EX_MAX_SZ ? i : EX_MAX_SZ - 1;
    e->nr = EX_SYNTAX_ERROR;
    return e->nr;
}

int ti_parseres_syntax_err_tpl(cleri_parse_t *pr, ex_t *e)
{
    if (pr->str[pr->pos] == '`' && pr->str[pr->pos+1] == '\0')
    {
        size_t line, pos;
        cleri_parse_expect_start(pr);
        cleri_parse_line_pos(pr, &line, &pos);
        ex_set(e, EX_SYNTAX_ERROR,
            "error at line %zu, position %zu, unexpeced end of template",
            line, pos);
        return e->nr;
    }
    return ti_parseres_syntax_err(pr, e);
}

