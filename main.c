#include <stdio.h>
#include <rql/kind.h>
#include <rql/prop.h>
#include <rql/rql.h>


int main(int argc, char * argv[])
{
    rql_t * rql = rql_create();

    rql_kind_t * kind = rql_kind_create("MyKind");

    rql_prop_via_t via = {0};
    via.def = rql_val_create(RQL_VAL_STR, "My String Value");
    rql_prop_t * prop = rql_prop_create("MyProp", RQL_VAL_STR, 0, via);

    printf("Pntr: %p, str: %s\n", prop->via.def, prop->via.def->str_);

    rql_kind_append_props(kind, &prop, 1);

    printf("Pntr: %p, str: %s\n", kind->props[0], kind->props[0]->via.def->str_);

    rql_kind_drop(kind);

    rql_destroy(rql);

    printf("Bye!\n");

    return 0;
}
