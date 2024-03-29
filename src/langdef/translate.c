/*
 * langdef/translate.c
 */
#include <langdef/translate.h>
#include <langdef/langdef.h>

const char * langdef_translate(cleri_t * elem)
{
    switch (elem->gid)
    {
    case CLERI_GID_K_BREAK:
    case CLERI_GID_K_CONTINUE:
    case CLERI_GID_OPR0_MUL_DIV_MOD:
    case CLERI_GID_OPR1_ADD_SUB:
    case CLERI_GID_OPR2_BITWISE_SHIFT:
    case CLERI_GID_OPR3_BITWISE_AND:
    case CLERI_GID_OPR4_BITWISE_XOR:
    case CLERI_GID_OPR5_BITWISE_OR:
    case CLERI_GID_OPR6_COMPARE:
    case CLERI_GID_OPR7_CMP_AND:
    case CLERI_GID_OPR8_CMP_OR:
    case CLERI_GID_T_FALSE:
    case CLERI_GID_T_INT:
    case CLERI_GID_T_NIL:
    case CLERI_GID_T_STRING:
    case CLERI_GID_T_TRUE:
    case CLERI_GID_X_ARRAY:
    case CLERI_GID_X_ASSIGN:
    case CLERI_GID_X_BLOCK:
    case CLERI_GID_X_CHAIN:
    case CLERI_GID_X_CLOSURE:
    case CLERI_GID_X_FUNCTION:
    case CLERI_GID_X_INDEX:
    case CLERI_GID_X_PARENTHESIS:
    case CLERI_GID_X_PREOPR:
    case CLERI_GID_X_TEMPLATE:
    case CLERI_GID_X_TERNARY:
    case CLERI_GID_X_THING:
        return "";
    }
    return NULL;
}
