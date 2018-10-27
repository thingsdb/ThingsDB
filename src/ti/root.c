//typedef enum
//{
//    TI_ROOT_UNDEFINED,
//    TI_ROOT_ROOT,
//    TI_ROOT_DATABASES,
//    TI_ROOT_DATABASES_CREATE,
//    TI_ROOT_DATABASES_DATABASE,
//    TI_ROOT_USERS,
//    TI_ROOT_CONFIG_REDUNDANCY,
//    TI_ROOT_CONFIG,
//} ti_root_enum;
//
//
//ti_root_enum * ti_root_by_strn(const char * str, const size_t n)
//{
//    if (strncmp("databases", str, n) == 0)
//        return TI_ROOT_DATABASES;
//
//    if (strncmp("users", str, n) == 0)
//        return TI_ROOT_USERS;
//
//    if (strncmp("config", str, n) == 0)
//        return TI_ROOT_CONFIG;
//
//    return TI_ROOT_UNDEFINED;
//}
//
//ex_enum ti_root_function(ti_root_enum scope, cleri_node_t * nd, , ex_t * e)
//{
//    cleri_t * function = nd                 /* sequence */
//            ->children->node                /* choice */
//            ->children->node->cl_obj;       /* keyword or identifier */
//
//    switch (scope)
//    {
//    case TI_ROOT_DATABASES:
//        switch(function->gid)
//        {
//        case CLERI_GID_F_CREATE:
//
//        }
//
//    }
//}
//
//static ex_enum query__root(ti_query_t * query, ti_root_enum scope, cleri_node_t * nd, ex_t * e)
//{
//    cleri_children_t * child;
//    cleri_t * obj = nd->cl_obj;
//
//    switch (obj->tp)
//    {
//    case CLERI_GID_IDENTIFIER:
//        switch (scope)
//        {
//        case TI_ROOT_ROOT:
//            switch ((scope = ti_root_by_strn(nd->str, nd->len)))
//            {
//            case TI_ROOT_DATABASES:
//            case TI_ROOT_USERS:
//            case TI_ROOT_CONFIG:
//                break;
//            default:
//                ex_set(e, EX_INDEX_ERROR,
//                        "`ThingsDB` has no property `%.*s`",
//                        nd->len,
//                        nd->str);
//                return e->nr;
//            }
//            break;
//        case TI_ROOT_DATABASES:
//            ex_set(e, EX_INDEX_ERROR,
//                    "`ThingsDB.databases` has no property `%.*s`",
//                    nd->len,
//                    nd->str);
//            return e->nr;
//        }
//        break;
//    case CLERI_GID_FUNCTION:
//    {
//
//        }
//    }
//
//    for (child = nd->children; child; child = child->next)
//        if (query__root(query, scope, child->node, e))
//            break;
//}
//
