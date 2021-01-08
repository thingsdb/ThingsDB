/*
 * ti/ext/async.c
 */
#define PY_SSIZE_T_CLEAN
#include <ctype.h>
#include <Python.h>
#include <datetime.h>
#include <ti.h>
#include <ti/datetime.h>
#include <ti/ext/py.h>
#include <ti/nil.h>
#include <ti/query.h>
#include <ti/varr.h>
#include <ti/vbool.h>
#include <ti/vfloat.h>
#include <ti/vint.h>
#include <util/strx.h>

/*
 * Do not allow deep nesting of objects. This should be avoided at any cost
 * because it may grow very rapidly once recursion is used.
 */
#define MAX_OBJECT_NESTING 6

static const char * ext_py__init_code =
    "import threading\n"
    "import _ti\n"
    "\n"
    "def _ti_work(pid, func, arg):\n"
    "    try:\n"
    "        result = func\n"
    "    except Exception as e:\n"
    "        result = e\n"
    "    finally:\n"
    "        _ti._eval_set_result(pid, result)\n"
    "\n"
    "def _ti_call_ext(pid, func, arg):\n"
    "    t = threading.Thread(\n"
    "        target=_ti_work,\n"
    "        args=(123, 456, 789))\n"
    "    t.start()\n";


PyObject * ext_py__datetime_from_dt_and_tz(ti_datetime_t * dt)
{
    PyObject * p_delta, * p_tzname, * p_tz, * p_ts,  * p_tuple, * p_datetime;
    struct tm tm;

    if (ti_datetime_time(dt, &tm))
        return NULL;

    if (tm.tm_gmtoff > INT_MAX || tm.tm_gmtoff < INT_MIN)
        return NULL;

    p_delta = PyDelta_FromDSU(0, (int) tm.tm_gmtoff, 0);
    if (!p_delta)
        return NULL;

    p_tzname = PyUnicode_FromStringAndSize(dt->tz->name, dt->tz->n);
    if (!p_tzname)
        goto fail0;

    p_tz = PyTimeZone_FromOffsetAndName(p_delta, p_tzname);
    if (!p_tz)
        goto fail1;

    Py_DECREF(p_tzname);
    Py_DECREF(p_delta);

    p_ts = PyLong_FromLongLong(DATETIME(dt));
    if (!p_ts)
        goto fail2;

    p_tuple = PyTuple_New(2);
    if (!p_tuple)
        goto fail3;

    /* this steals references to p_ts and p_tz */
    (void) PyTuple_SET_ITEM(p_tuple, 0, p_ts);
    (void) PyTuple_SET_ITEM(p_tuple, 1, p_tz);

    p_datetime = PyDateTime_FromTimestamp(p_tuple);
    Py_DECREF(p_tuple);
    return p_datetime;

fail3:
    Py_DECREF(p_ts);
fail2:
    Py_DECREF(p_tz);
    return NULL;
fail1:
    Py_DECREF(p_tzname);
fail0:
    Py_DECREF(p_delta);
    return NULL;
}

PyObject * ext_py__datetime_from_dt_and_offset(ti_datetime_t * dt)
{
    PyObject * p_delta, * p_tz, * p_ts,  * p_tuple, * p_datetime;

    p_delta = PyDelta_FromDSU(0, (int) dt->offset * 60, 0);
    if (!p_delta)
        return NULL;

    p_tz = PyTimeZone_FromOffset(p_delta);
    if (!p_tz)
        goto fail0;

    Py_DECREF(p_delta);

    p_ts = PyLong_FromLongLong(DATETIME(dt));
    if (!p_ts)
        goto fail1;

    p_tuple = PyTuple_New(2);
    if (!p_tuple)
        goto fail2;

    /* this steals references to p_ts and p_tz */
    (void) PyTuple_SET_ITEM(p_tuple, 0, p_ts);
    (void) PyTuple_SET_ITEM(p_tuple, 1, p_tz);

    p_datetime = PyDateTime_FromTimestamp(p_tuple);
    Py_DECREF(p_tuple);
    return p_datetime;

fail2:
    Py_DECREF(p_ts);
fail1:
    Py_DECREF(p_tz);
    return NULL;
fail0:
    Py_DECREF(p_delta);
    return NULL;
}

PyObject * ext_py__datetime_from_dt_utc(ti_datetime_t * dt)
{
    PyObject * p_ts,  * p_tuple, * p_datetime;

    p_ts = PyLong_FromLongLong(DATETIME(dt));
    if (!p_ts)
        return NULL;

    p_tuple = PyTuple_New(1);
    if (!p_tuple)
        goto fail0;

    /* this steals references to p_ts and p_tz */
    (void) PyTuple_SET_ITEM(p_tuple, 0, p_ts);

    p_datetime = PyDateTime_FromTimestamp(p_tuple);
    Py_DECREF(p_tuple);
    return p_datetime;

fail0:
    Py_DECREF(p_ts);
    return NULL;
}

PyObject * ext_py__datetime_from_dt(ti_datetime_t * dt)
{
    return dt->tz
            ? ext_py__datetime_from_dt_and_tz(dt)
            : dt->tz
            ? ext_py__datetime_from_dt_and_offset(dt)
            : ext_py__datetime_from_dt_utc(dt);
}

PyObject * ext_py__py_from_val(ti_val_t * val, ex_t * e, int depth)
{
    if (++depth == MAX_OBJECT_NESTING)
    {
        ex_set(e, EX_OPERATION_ERROR,
            "object has reached the maximum depth of %d",
            MAX_OBJECT_NESTING);
        return NULL;
    }

    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
        Py_RETURN_NONE;
    case TI_VAL_INT:
    {
        PyObject * p_val = PyLong_FromLongLong(VINT(val));
        if (!p_val)
            ex_set_mem(e);
        return p_val;
    }
    case TI_VAL_FLOAT:
    {
        PyObject * p_val = PyLong_FromDouble(VFLOAT(val));
        if (!p_val)
            ex_set_mem(e);
        return p_val;
    }
    case TI_VAL_BOOL:
        return VBOOL(val)
                ? Py_INCREF(Py_True), Py_True
                : Py_INCREF(Py_False), Py_False;
    case TI_VAL_DATETIME:
    {
        PyObject * p_val = ext_py__datetime_from_dt((ti_datetime_t *) val);
        if (!p_val)
            ex_set(e, EX_VALUE_ERROR, "failed to convert date/time type");
        return p_val;
    }
    case TI_VAL_NAME:
    {
        ti_name_t * name = (ti_name_t *) val;
        PyObject * p_val = PyUnicode_FromStringAndSize(name->str, name->n);
        if (!p_val)
            ex_set_mem(e);
        return p_val;
    }
    case TI_VAL_STR:
    {
        ti_raw_t * raw = (ti_raw_t *) val;
        if (strx_is_utf8n((const char *) raw->data, raw->n))
        {
            PyObject * p_val = PyUnicode_FromStringAndSize(
                    (const char *) raw->data,
                    raw->n);
            if (!p_val)
                ex_set_mem(e);
            return p_val;
        }
    }
    /* fall through */
    case TI_VAL_BYTES:
    {
        ti_raw_t * raw = (ti_raw_t *) val;
        PyObject * p_val = PyBytes_FromStringAndSize(
                (const char *) raw->data,
                raw->n);
        if (!p_val)
            ex_set_mem(e);
        return p_val;
    }
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_MP:
    case TI_VAL_FUTURE:
    case TI_VAL_TEMPLATE:
    case TI_VAL_CLOSURE:
    case TI_VAL_REGEX:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `%s` to a Python object",
                ti_val_str(val));
        return NULL;
    }
    assert (0);
    Py_RETURN_NONE;
}

/*
 * Create set_result function
 */
static PyObject * ti_set_result(PyObject * UNUSED(self), PyObject * args)
{
    ex_t e = {0};
    ti_future_t * future;
    PyObject * p_result;
    intptr_t ptr;
    size_t idx = 0;

    if (!PyArg_ParseTuple(args, "LO", &ptr, &p_result))
    {
        log_error("set_result(..) called by using wrong arguments");
        Py_RETURN_NONE;
    }

    /*
     * Check to be sure that the pointer is an actual registered future
     * pointer. If not checked, ThingsDB might break on receiving a
     * corrupted pointer.
     */
    future = (ti_future_t *) ptr;
    for (vec_each(ti.futures, ti_future_t, f), ++idx)
        if (future == f)
            break;

    if (idx == ti.futures->n)
    {
        log_error(
                "set_result(..) is called with an unknown future pointer: %p",
                future);
        Py_RETURN_NONE;
    }

    future = vec_swap_remove(ti.futures, idx);

    future->rval = (ti_val_t *) ti_varr_from_vec(future->args);
    if (future->rval)
    {
        future->args = NULL;
        /* TODO: replace the first value with the p_return value */
    }
    else
        ex_set_mem(&e);


    ti_query_on_future_result(future, &e);

    Py_RETURN_NONE;
}

static PyMethodDef TiMethods[] = {
    {"set_result", ti_set_result, METH_VARARGS,
     "Can be called only once to set the result for ThingsDB."},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef TiModule = {
    PyModuleDef_HEAD_INIT, "_ti", NULL, -1, TiMethods,
    NULL, NULL, NULL, NULL
};

static PyObject * PyInit_ti(void)
{
    return PyModule_Create(&TiModule);
}

static int ext_py__insert_module_path(void)
{
    int rc = -1;
    char * s = ti.cfg->py_modules_path;
    PyObject * sys_module, * p_path, * p_insert, * p_zero;
    size_t n;

    if (!s)
        return 0;  /* no additional modules path is configured */

    sys_module = PyImport_ImportModule("sys");
    if (!sys_module)
    {
        log_error("(py) cannot import `sys` module");
        goto fail0;
    }

    p_path = PyObject_GetAttrString(sys_module, "path");
    if (!p_path)
    {
        log_error("(py) cannot import `path` from `sys`");
        goto fail1;
    }

    p_insert = PyObject_GetAttrString(p_path, "insert");
    if (!p_insert)
    {
        log_error("(py) cannot load `insert` from `sys.path`");
        goto fail2;
    }

    p_zero = PyLong_FromLong(0);
    if (!p_zero)
    {
        log_error("(py) failed to create long");
        goto fail3;
    }

    while (*s)
    {
        PyObject * p_tuple, * p_mod_path, * p_ret;
        char * path_str;

        /* skip white space */
        for (;isspace(*s); ++s);

        /* set begin path and read the size of the path */
        path_str = s;
        for (n = 0; *s && *s != ',' && *s != ';'; ++n, ++s);
        if (*s)
            ++s;  /* not reached the end, skip divider */
        if (!n)
            continue;  /* no length */

        p_mod_path = PyUnicode_DecodeFSDefaultAndSize(path_str, n);
        if (!p_mod_path)
        {
            log_error("(py) failed to create path string");
            goto fail4;
        }

        p_tuple = PyTuple_New(2);
        if (!p_tuple)
        {
            Py_DECREF(p_mod_path);
            log_error("(py) failed to create tuple");
            goto fail4;
        }

        /* get a reference for 0 */
        Py_INCREF(p_zero);

        /* this steals references to p_zero and p_path */
        (void) PyTuple_SET_ITEM(p_tuple, 0, p_zero);
        (void) PyTuple_SET_ITEM(p_tuple, 1, p_mod_path);

        p_ret = PyObject_CallObject(p_insert, p_tuple);
        Py_DECREF(p_tuple);

        if (!p_ret)
        {
            log_error("(py) failed to call sys.path.insert(..)");
            goto fail4;
        }

        Py_DECREF(p_ret);
    }

    rc = 0;

fail4:
    Py_DECREF(p_zero);
fail3:
    Py_DECREF(p_insert);
fail2:
    Py_DECREF(p_path);
fail1:
    Py_DECREF(sys_module);
fail0:
    return rc;
}

static void ext_py__destroy_cb(PyObject * p_main)
{
    Py_DECREF(p_main);
}

static void ext_py__load_modules(void)
{
    char * s = ti.cfg->py_modules;
    while (*s)
    {
        ti_ext_t * ext;
        PyObject * p_module, * p_name, * p_main;
        char * module_str;
        const char * name_str;

        /* skip white space */
        for (;isspace(*s); ++s);

        /* set begin path and read the size of the path */
        module_str = s;
        for (; *s && *s != ',' && *s != ';'; ++s);

        if (*s)
        {
            *s = '\0';
            ++s;  /* not reached the end, skip divider */
        }

        if (!*module_str)
            continue;

        p_module = PyImport_ImportModule(module_str);
        if (!p_module)
        {
            log_error(
                    "(py) failed to load module `%s`; "
                    "make sure the correct module path is configured",
                    module_str);
            continue;
        }

        p_name = PyObject_GetAttrString(p_module, "THINGSDB_EXT_NAME");
        if (!p_name)
        {
            log_error(
                    "(py) failed to load module `%s`; "
                    "missing `THINGSDB_EXT_NAME`",
                    module_str);
            goto fail0;
        }

        if (!PyUnicode_Check(p_name))
        {
            log_error(
                    "(py) failed to load module `%s`; "
                    "`THINGSDB_EXT_NAME` must be a Python Unicode String",
                    module_str);
            goto fail1;
        }

        p_main = PyObject_GetAttrString(p_module, "main");
        if (!p_main)
        {
            log_error(
                    "(py) failed to load module `%s`; "
                    "missing `main` function`",
                    module_str);
            goto fail1;
        }

        if (!PyCallable_Check(p_main))
        {
            log_error(
                    "(py) failed to load module `%s`; "
                    "`main` must be callable",
                    module_str);
            goto fail2;
        }

        ext = malloc(sizeof(ti_ext_t));
        if (!ext)
        {
            log_error(EX_MEMORY_S);
            goto fail2;
        }

        ext->cb = (ti_ext_cb) ti_ext_py_cb;
        ext->destroy_cb = (ti_ext_destroy_cb) ext_py__destroy_cb;
        ext->data = p_main;

        name_str = PyUnicode_AsUTF8(p_name);

        switch(smap_add(ti.extensions, name_str, ext))
        {
        case SMAP_SUCCESS:
            goto done;
        case SMAP_ERR_EMPTY_KEY:
            log_error(
                    "(py) failed to load module `%s`; "
                    "`THINGSDB_EXT_NAME` must be not be empty",
                    module_str);
            break;
        case SMAP_ERR_ALLOC:
            log_error(EX_MEMORY_S);
            break;
        case SMAP_ERR_EXIST:
            log_error(
                    "(py) failed to load module `%s`; "
                    "cannot register `%s` as another extension with the same "
                    "name already exists",
                    module_str, name_str);
            break;
        }

        free(ext);
fail2:
        Py_DECREF(p_main);
fail1:
done:
        Py_DECREF(p_name);
fail0:
        Py_DECREF(p_module);
    }
}

static PyObject * py_ext_call_ext;

static int py_ext__init_call_ext(void)
{
    PyObject * p_main_module;

    p_main_module = PyImport_ImportModule("__main__");
    if (!p_main_module)
    {
        log_error("(py) failed to load main module");
        return -1;

    }

    py_ext_call_ext = PyObject_GetAttrString(p_main_module, "_ti_call_ext");
    Py_DECREF(p_main_module);

    if (!py_ext_call_ext)
    {
        log_error("(py) failed to load main module");
        return -1;
    }

    if (!PyCallable_Check(py_ext_call_ext))
    {
        log_error("(py) `_ti_call_ext` is not callable");
        return -1;
    }

    Py_INCREF(py_ext_call_ext);

    return 0;
}

int ti_ext_py_init(void)
{
    if (PyImport_AppendInittab("_ti", &PyInit_ti) < 0)
    {
        log_error("(py) failed to load _ti module");
        return -1;
    }

    Py_Initialize();

    if (ext_py__insert_module_path())
        goto fail;

    if (PyRun_SimpleString(ext_py__init_code))
    {
        log_error("(py) failed to run initial code");
        goto fail;
    }

    if (py_ext__init_call_ext())
        goto fail;

    /* load modules, failed modules are logged and skipped */
    ext_py__load_modules();

    return 0;

fail:
    (void) Py_FinalizeEx();
    return -1;
}

void ti_ext_py_destroy(void)
{
    if (py_ext_call_ext)
    {
        Py_DECREF(py_ext_call_ext);
        (void) Py_FinalizeEx();
    }
}

void ti_ext_py_cb(ti_future_t * future, void * data)
{
    ex_t e = {0};
    intptr_t ptr = (intptr_t) future;
    PyObject * p_arg, * p_main, * p_tuple, * p_return, * p_pid;

    p_main = data;

    if (future->args->n)
    {
        ti_val_t * val = vec_first(future->args);
        p_arg = ext_py__py_from_val(val, &e, 0);
        if (!p_arg)
            goto fail0;
    }
    else
    {
        p_arg = Py_None;
        Py_INCREF(p_arg);
    }

    p_pid = PyLong_FromLongLong(ptr);
    if (!p_pid)
    {
        ex_set_mem(&e);
        goto fail1;
    }

    if (vec_push_create(&ti.futures, future))
    {
        ex_set_mem(&e);
        goto fail2;
    }

    p_tuple = PyTuple_New(3);
    if (!p_tuple)
    {
        ex_set_mem(&e);
        goto fail3;
    }

    /* this steals references to p_ts and p_tz */
    (void) PyTuple_SET_ITEM(p_tuple, 0, p_pid);
    (void) PyTuple_SET_ITEM(p_tuple, 1, p_main);
    (void) PyTuple_SET_ITEM(p_tuple, 2, p_arg);

    LOGC("call function...");
    p_return = PyObject_CallObject(py_ext_call_ext, p_tuple);
    LOGC("done function...");
    Py_DECREF(p_tuple);

    if (p_return)
    {
        Py_DECREF(p_return);
        return;  /* success, ti_query_on_future_result will be called
                             once the python thread finishes */
    }

    ex_set_internal(&e);
fail3:
    vec_pop(ti.futures);
fail2:
    Py_DECREF(p_pid);
fail1:
    Py_DECREF(p_arg);
fail0:
    ti_query_on_future_result(future, &e);
}

