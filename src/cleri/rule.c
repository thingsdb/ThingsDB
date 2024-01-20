/*
 * rule.c - cleri regular rule element. (do not directly use this element but
 *          create a 'prio' instead which will be wrapped by a rule element)
 */
#include <cleri/rule.h>
#include <stdlib.h>

static void rule__free(cleri_t * cl_object);
static cleri_node_t * rule__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * rule);
static void rule__tested_free(cleri_rule_tested_t * tested);

/*
 * Returns NULL in case an error has occurred.
 */
cleri_t * cleri__rule(uint32_t gid, cleri_t * cl_obj)
{
    if (cl_obj == NULL)
    {
        return NULL;
    }

    cleri_t * cl_object = cleri_new(
            gid,
            CLERI_TP_RULE,
            &rule__free,
            &rule__parse);

    if (cl_object != NULL)
    {
        cl_object->via.rule = cleri__malloc(cleri_rule_t);

        if (cl_object->via.rule == NULL)
        {
            free(cl_object);
            cl_object = NULL;
        }
        else
        {
            cl_object->via.rule->cl_obj = cl_obj;
            cleri_incref(cl_obj);
        }
    }

    return cl_object;
}

/*
 * Initialize a rule and return the test result.
 * Result can be either CLERI_RULE_TRUE, CLERI_RULE_FALSE or CLERI_RULE_ERROR.
 *
 *  - CLERI_RULE_TRUE: a new test is created
 *  - CLERI_RULE_FALSE: no new test is created
 *  - CLERI_RULE_ERROR: an error occurred
 */
cleri_rule_test_t cleri__rule_init(
        cleri_rule_tested_t ** target,
        cleri_rule_tested_t * tested,
        const char * str)
{
    /*
     * return true (1) when a new test is created, false (0) when not.
     */
    (*target) = tested;

    if ((*target)->str == NULL)
    {
        (*target)->str = str;
        return CLERI_RULE_TRUE;
    }

    if ((*target)->str == str)
    {
        return CLERI_RULE_FALSE;
    }

    while (((*target) = (*target)->next) != NULL && str <= (*target)->str)
    {
        if ((*target)->str == str)
        {
            return CLERI_RULE_FALSE;
        }
        tested = (*target);
    }

    *target = cleri__malloc(cleri_rule_tested_t);

    if (*target == NULL)
    {
        return CLERI_RULE_ERROR;
    }

    (*target)->str = str;
    (*target)->node = NULL;
    (*target)->next = tested->next;
    tested->next = *target;
    return CLERI_RULE_TRUE;
}

static void rule__free(cleri_t * cl_object)
{
    cleri_free(cl_object->via.rule->cl_obj);
    free(cl_object->via.rule);
}

/*
 * Returns a node or NULL. In case of an error pr->is_valid is set to -1.
 */
static cleri_node_t * rule__parse(
        cleri_parse_t * pr,
        cleri_node_t * parent,
        cleri_t * cl_obj,
        cleri_rule_store_t * __rule __attribute__((unused)))
{
    cleri_rule_store_t nrule;
    cleri_node_t * node;
    cleri_node_t * rnode;

    if (pr->flags & CLERI_FLAG_EXCLUDE_RULE_THIS)
    {
        nrule.depth = 0;
        nrule.tested.str = NULL;
        nrule.tested.node = NULL;
        nrule.tested.next = NULL;
        nrule.root_obj = cl_obj->via.rule->cl_obj;

        node = cleri__parse_walk(
                pr,
                parent,
                nrule.root_obj,
                &nrule,
                CLERI__EXP_MODE_REQUIRED);

        if (node != NULL)
        {
            node = parent;
        }

        /* cleanup rule */
        rule__tested_free(&nrule.tested);

        return node;
    }

    if ((node = cleri__node_new(cl_obj, parent->str + parent->len, 0)) == NULL)
    {
        pr->is_valid = -1;
        return NULL;
    }

    nrule.depth = 0;
    nrule.tested.str = NULL;
    nrule.tested.node = NULL;
    nrule.tested.next = NULL;
    nrule.root_obj = cl_obj->via.rule->cl_obj;

    rnode = cleri__parse_walk(
            pr,
            node,
            nrule.root_obj,
            &nrule,
            CLERI__EXP_MODE_REQUIRED);

    if (rnode == NULL)
    {
        cleri__node_free(node);
        node = NULL;
    }
    else
    {
        parent->len += node->len;
        cleri__node_add(parent, node);
    }

    /* cleanup rule */
    rule__tested_free(&nrule.tested);

    return node;
}

/*
 * Cleanup rule tested
 */
static void rule__tested_free(cleri_rule_tested_t * tested)
{
    cleri_rule_tested_t * next = tested->next;
    cleri__node_free(tested->node);
    while (next != NULL)
    {
        tested = next->next;
        cleri__node_free(next->node);
        free(next);
        next = tested;
    }
}

