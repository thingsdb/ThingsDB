/*
 * regex.c - cleri regular expression element.
 */
#include <cleri/regex.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void regex__free(cleri_t * cl_object);

static cleri_node_t *  regex__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);

/*
 * Returns a regex object or NULL in case of an error.
 *
 * Argument pattern must start with character '^'. Be sure to check the pattern
 * for the '^' character before calling this function.
 *
 * Warning: this function could write to stderr in case the pattern could not
 * be compiled.
 */
cleri_t * cleri_regex(uint32_t gid, const char * pattern)
{
    cleri_t * cl_object;
    int pcre_error_num;
    PCRE2_SIZE pcre_error_offset;

    /*
     * starting with ^ is not required as flags may go first
     * assert (pattern[0] == '^');
     */

    cl_object = cleri_new(
            gid,
            CLERI_TP_REGEX,
            &regex__free,
            &regex__parse);

    if (cl_object == NULL)
    {
        return NULL;
    }

    cl_object->via.regex = cleri__malloc(cleri_regex_t);

    if (cl_object->via.regex == NULL)
    {
        free(cl_object);
        return NULL;
    }

    cl_object->via.regex->regex = pcre2_compile(
            (PCRE2_SPTR8) pattern,
            PCRE2_ZERO_TERMINATED,
            0,
            &pcre_error_num,
            &pcre_error_offset,
            NULL);

    if(cl_object->via.regex->regex == NULL)
    {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(pcre_error_num, buffer, sizeof(buffer));

        fprintf(stderr,
                "error: cannot compile '%s' (%s)\n",
                pattern,
                buffer);
        free(cl_object->via.regex);
        free(cl_object);
        return NULL;
    }

    cl_object->via.regex->match_data = pcre2_match_data_create_from_pattern(
            cl_object->via.regex->regex,
            NULL);

    if (cl_object->via.regex->match_data == NULL)
    {
        pcre2_code_free(cl_object->via.regex->regex);
        fprintf(stderr, "error: cannot create match data\n");
        free(cl_object->via.regex);
        free(cl_object);
        return NULL;
        return NULL;
    }

    return cl_object;
}

/*
 * Destroy regex object.
 */
static void regex__free(cleri_t * cl_object)
{
    pcre2_match_data_free(cl_object->via.regex->match_data);
    pcre2_code_free(cl_object->via.regex->regex);
    free(cl_object->via.regex);
}

/*
 * Returns a node or NULL. In case of an error, pr->is_valid is set to -1
 */
static cleri_node_t *  regex__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule __attribute__((unused)))
{
    int pcre_exec_ret;
    PCRE2_SIZE * ovector;
    const char * str = parent->str + parent->len;
    cleri_node_t * node;

    pcre_exec_ret = pcre2_match(
            cl_obj->via.regex->regex,
            (PCRE2_SPTR8) str,
            PCRE2_ZERO_TERMINATED,
            0,                     // start looking at this point
            0,                     // OPTIONS
            cl_obj->via.regex->match_data,
            NULL);

    if (pcre_exec_ret < 0)
    {
        if (cleri__expecting_update(pr->expecting, cl_obj, str) == -1)
        {
            pr->is_valid = -1; /* error occurred */
        }
        return NULL;
    }
    ovector = pcre2_get_ovector_pointer(cl_obj->via.regex->match_data);

    /* since each regex pattern should start with ^ we now sub_str_vec[0]
     * should be 0. sub_str_vec[1] contains the end position in the sting
     */
    if ((node = cleri__node_new(cl_obj, str, (size_t) ovector[1])) != NULL)
    {
        parent->len += node->len;
        cleri__node_add(parent, node);
    }
    else
    {
        pr->is_valid = -1;  /* error occurred */
    }

    return node;
}
