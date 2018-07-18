#include <locale.h>
#include <stdlib.h>
#include <time.h>
#include <Python.h>
#include <rql/rql.h>
#include <rql/store.h>
#include <rql/user.h>
#include <rql/version.h>
#include <util/fx.h>


const char * pycode =
    "import sys\n"
    "sys.path.insert(0, './')\n";

const char * pycode2 =
    "import sys\n"
    "print(sys.path)\n";

const char * pycode3 =
    "import fact\n"
    "print(dir(fact))\n";


int main(int argc, char * argv[])
{
    int rc = 0;

    Py_Initialize();
    PyObject * main_module;
    PyObject * main_dict;
    PyObject * sys_module;
    PyObject * sys_dict;
    PyObject * version_obj;
    char * version_string;

    main_module = PyImport_ImportModule("__main__");
    main_dict = PyModule_GetDict(main_module);

    /* Fetch the sys module */
    sys_module = PyImport_ImportModule("sys");
    sys_dict   = PyModule_GetDict(sys_module);

    /* Attach the sys module into the __main__ namespace */
    PyDict_SetItemString(main_dict, "sys", sys_module);

    /* Retrieve the Python version from sys and print it out */
    version_obj = PyMapping_GetItemString(sys_dict, "version");
    version_string = PyUnicode_AsUTF8(version_obj);
    fprintf(stderr, "%s\n\n", version_string);
    Py_XDECREF(version_obj);

    PyRun_SimpleString(pycode);
    PyRun_SimpleString(pycode2);
    PyRun_SimpleString(pycode3);

    /*
     * set local to LC_ALL
     * more info at: http://www.cprogramming.com/tutorial/unicode.html
     */
    setlocale(LC_ALL, "");

    /* initialize random */
    srand(time(NULL));

    /* set threadpool size to 4 (default=4) */
    putenv("UV_THREADPOOL_SIZE=4");

    /* set default timezone to UTC */
    putenv("TZ=:UTC");
    tzset();

    rql_t * rql = rql_create();

    /* check rql and parse arguments */
    if (!rql || (rc = rql_args_parse(rql->args, argc, argv))) goto stop;

    if (rql->args->version)
    {
        rql_version_print();
        goto stop;
    }

    rql_init_logger(rql);

    if ((rc = rql_cfg_parse(rql->cfg, rql->args->config))) goto stop;
    if ((rc = rql_lock(rql))) goto stop;
    if ((rc = rql_init_fn(rql))) goto stop;

    if (rql->args->init)
    {
        if (fx_file_exist(rql->fn))
        {
            printf("error: directory '%s' is already initialized\n",
                    rql->cfg->rql_path);
            rc = -1;
            goto stop;
        }
        if ((rc = rql_build(rql)))
        {
            printf("error: building new pool has failed\n");
            goto stop;
        }

        printf(
            "Well done! You successfully initialized a new rql pool.\n\n"
            "You can now star RQL and connect by using the default user `%s`.\n"
            "..before I forget, the password is '%s'\n\n",
            rql_user_def_name,
            rql_user_def_pass);

        goto stop;
    }
    else if (strlen(rql->args->secret))
    {
        printf(
            "Waiting for a request to join some pool of rql nodes...\n"
            "(if you want to create a new pool instead, press CTRL+C and "
            "use the --init argument)\n");
    }
    else if (fx_file_exist(rql->fn))
    {
        if ((rc = rql_read(rql)))
        {
            printf("error reading rql pool from: '%s'\n", rql->fn);
            goto stop;
        }

        if ((rc = rql_restore(rql)))
        {
            printf("error loading rql pool\n");
            goto stop;
        }
    }
    else
    {
        printf(
            "You should either create a new pool using the --init argument "
            "or set a one-time-secret using the --secret argument and wait "
            "for a request from another node to join.\n");
        goto stop;
    }

    rc = rql_run(rql);
stop:
    if (!rc && rql_unlock(rql))
    {
        rc = -1;
    }
    rql_destroy(rql);

    if (Py_FinalizeEx() < 0)
        return 120;

    return rc;
}
