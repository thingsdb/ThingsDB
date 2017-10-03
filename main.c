#include <stdio.h>
#include <rql/kind.h>
#include <rql/prop.h>
#include <rql/rql.h>


int main(int argc, char * argv[])
{
    rql_t * rql = rql_create();

    rql_kind_t * kind = rql_kind_create("MyKind");

    rql_val_via_t * val = rql_val_create(RQL_VAL_STR, "My String Value");
    rql_prop_t * prop = rql_prop_create("MyProp", RQL_VAL_STR, 0, NULL, NULL, val);

    printf("Pntr: %p, str: %s\n", prop->def, prop->def->str_);

    rql_kind_append_props(kind, &prop, 1);

    printf("Pntr: %p, str: %s\n", vec_get(kind->props, 0), ((rql_prop_t *) vec_get(kind->props, 0))->def->str_);

    rql_kind_drop(kind);

    rql_destroy(rql);

    printf("Bye!\n");

    return 0;
}
