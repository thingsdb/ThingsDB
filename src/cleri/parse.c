/*
 * parse.c - this contains everything for parsing a string to a grammar.
 */
#include <cleri/expecting.h>
#include <cleri/parse.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static inline uint32_t parse__whitespace(cleri_parse_t * pr, const char * str);

/*
 * Return a parse result. In case of a memory allocation error the return value
 * will be NULL.
 */
cleri_parse_t * cleri_parse2(
    cleri_grammar_t * grammar,
    const char * str,
    int flags)
{
    cleri_node_t * nd;
    cleri_parse_t * pr;
    const char * end;

    /* prepare parsing */
    pr = cleri__malloc(cleri_parse_t);
    if (pr == NULL)
    {
        return NULL;
    }

    pr->flags = flags;
    pr->str = str;
    pr->tree = NULL;
    pr->kwcache = NULL;
    pr->expecting = NULL;
    pr->is_valid = 0;
    pr->grammar = grammar;

    if (    (pr->tree = cleri__node_new(NULL, str, 0)) == NULL ||
            (pr->kwcache = cleri__kwcache_new(str)) == NULL ||
            (pr->expecting = cleri__expecting_new(str, flags)) == NULL)
    {
        cleri_parse_free(pr);
        return NULL;
    }

    /* do the actual parsing */
    nd = cleri__parse_walk(
            pr,
            pr->tree,
            grammar->start,
            NULL,
            CLERI__EXP_MODE_REQUIRED);

    /* When is_valid is -1, an allocation error has occurred. */
    if (pr->is_valid == -1)
    {
        cleri_parse_free(pr);
        return NULL;
    }

    pr->is_valid = nd != NULL;

    /* process the parse result */
    end = pr->tree->str + pr->tree->len;

    /* check if we are at the end of the string */
    if (pr->is_valid && *end)
    {
        end += parse__whitespace(pr, end);
        if (*end)
        {
            pr->is_valid = false;
            if (pr->expecting->required)
            {
                if (cleri__expecting_set_mode(
                        pr->expecting,
                        end,
                        CLERI__EXP_MODE_REQUIRED) == -1 ||
                    cleri__expecting_update(
                        pr->expecting,
                        CLERI_END_OF_STATEMENT,
                        end) == -1)
                {
                    cleri_parse_free(pr);
                    return NULL;
                }
                cleri__expecting_combine(pr->expecting);
            }
            else if (cleri__expecting_update(
                        pr->expecting,
                        CLERI_END_OF_STATEMENT,
                        end))
            {
                cleri_parse_free(pr);
                return NULL;
            }
        }
    }

    pr->pos = pr->is_valid
        ? pr->tree->len
        : (size_t) (pr->expecting->str - pr->str);

    cleri__olist_unique(pr->expecting->required);
    pr->expect = pr->expecting->required;

    return pr;
}

/*
 * Destroy parser. (parsing NULL is allowed)
 */
void cleri_parse_free(cleri_parse_t * pr)
{
    cleri__node_free(pr->tree);
    free(pr->kwcache);
    if (pr->expecting != NULL)
    {
        cleri__expecting_free(pr->expecting);
    }
    free(pr);
}

/*
 * Reset expect to start
 */
void cleri_parse_expect_start(cleri_parse_t * pr)
{
    pr->expect = pr->expecting->required;
}

static void parse__line_pos(cleri_parse_t * pr, size_t * line, size_t * pos)
{
    size_t n = pr->pos;
    const char * pt = pr->str;
    *pos = 0;
    *line = 1;
    while (n--)
    {
        if (*pt == '\n')
        {
            if (!n)
                break;

            ++pt;
            if (*pt == '\r')
            {
                if (!--n)
                    break;
                ++pt;
            }

            ++(*line);
            *pos = 0;
            continue;
        }
        if (*pt == '\r')
        {
            if (!n)
                break;

            ++pt;
            if (*(++pt) == '\n' )
            {
                if (!--n)
                    break;

                ++pt;
                ++(*line);
                *pos = 0;
            }

            ++(*line);
            *pos = 0;
            continue;
        }
        ++(*pos);
        ++pt;
    }
}

/*
 * Print parse result to a string. The return value is equal to the snprintf
 * function. Argument `translate_cb` maybe NULL or a function which may return
 * a string based on the `cleri_t`. This allows for returning nice strings for
 * regular expressions. The function may return NULL if it has no translation
 * for the given regular expression.
 */
int cleri_parse_strn(
    char * s,
    size_t n,
    cleri_parse_t * pr,
    cleri_translate_t * translate)
{
    int rc, count = 0;
    size_t i, m, line, pos;
    cleri_t * o;
    const char * expect;
    const char * template;
    if (pr == NULL)
    {
        return snprintf(s, n,
            "no parse result, a possible reason might be that the maximum "
            "recursion depth of %d has been reached", MAX_RECURSION_DEPTH);
    }
    if (pr->is_valid)
    {
        return snprintf(s, n, "parsed successfully");
    }
    /* make sure expecting is at start */
    cleri_parse_expect_start(pr);

    parse__line_pos(pr, &line, &pos);

    rc = snprintf(s, n, "error at line %zu, position %zu", line, pos);
    if (rc < 0)
    {
        return rc;
    }
    i = rc;

    expect = pr->str + pr->pos;
    if (isgraph(*expect))
    {
        size_t nc = cleri__kwcache_match(pr, expect);
        const char * pt = expect;
        const unsigned int max_chars = 20;

        if (nc < 1)
        {
            while (isdigit(*pt))
                ++pt;
            nc = pt - expect;
        }

        m = (i < n) ? n-i : 0;

        if (nc > 1)
        {
            rc = nc > max_chars
                ? snprintf(s+i, m, ", unexpected `%.*s...`", max_chars, expect)
                : snprintf(s+i, m, ", unexpected `%.*s`", (int) nc, expect);
        }
        else
        {
            rc = snprintf(s+i, m, ", unexpected character `%c`", *expect);
        }

        if (rc < 0)
        {
            return rc;
        }

        i += rc;
    }


    while (pr->expect)
    {
        o = pr->expect->cl_obj;
        if (!translate || !(expect = (*translate)(o))) switch(o->tp)
        {
        case CLERI_TP_END_OF_STATEMENT:
            expect = "end_of_statement";
            break;
        case CLERI_TP_KEYWORD:
            expect = o->via.keyword->keyword;
            break;
        case CLERI_TP_TOKENS:
            expect = o->via.tokens->spaced;
            break;
        case CLERI_TP_TOKEN:
            expect = o->via.token->token;
            break;
        default:
            expect = "";  /* continue */
        }

        if (*expect == '\0')
        {
            /* ignore empty strings */
            pr->expect = pr->expect->next;
            continue;
        }

        /* make sure len is not greater than the maximum size */
        m = (i < n) ? n-i : 0;

        /* we use count = 0 to print the first one, then for the others
         * a comma prefix and the last with -or-
         *
         * TODO: could be improved since the last `or` might never be added
         */
        template = !count++
            ? ", expecting: %s"
            : pr->expect->next
            ? ", %s"
            : " or %s";
        rc = snprintf(s+i, m, template, expect);
        if (rc < 0)
        {
            return rc;
        }
        i += rc;

        pr->expect = pr->expect->next;
    }
    return (int) i;
}

/*
 * Walk a parser object.
 * (recursive function, called from each parse_object function)
 * Returns a node or NULL. (In case of error one should check pr->is_valid)
 */
cleri_node_t * cleri__parse_walk(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule,
        int mode)
{
    /* set parent len to next none white space char */
    parent->len += parse__whitespace(pr, parent->str + parent->len);

    /* set expecting mode */
    if (cleri__expecting_set_mode(pr->expecting, parent->str, mode) == -1)
    {
        pr->is_valid = -1;
        return NULL;
    }

    /* note that the actual node is returned or NULL but we do not
     * actually need the node. (boolean true/false would be enough)
     */
    return (*cl_obj->parse_object)(pr, parent, cl_obj, rule);
}

static inline uint32_t parse__whitespace(cleri_parse_t * pr, const char * str)
{
    int pcre_exec_ret = pcre2_match(
                pr->grammar->re_whitespace,
                (PCRE2_SPTR8) str,
                PCRE2_ZERO_TERMINATED,
                0,                     // start looking at this point
                PCRE2_ANCHORED,        // OPTIONS
                pr->grammar->md_whitespace,
                NULL);

    return pcre_exec_ret < 0
        ? 0
        : pcre2_get_ovector_pointer(pr->grammar->md_whitespace)[1];
}
