/*
 * ti/ext/async.c
 */
#define PY_SSIZE_T_CLEAN
#include <ti/ext/async.h>
#include <ti/varr.h>
#include <ti/nil.h>
#include <ti/vint.h>
#include <ti/vfloat.h>
#include <ti/vbool.h>
#include <ti/query.h>
#include <ti.h>
#include <Python.h>
#include <ctype.h>


static const char * ext_py__init_code =
    "import threading\n"
    "import _ti\n"
    "\n"
    "def _ti_work(pid, func, *args):\n"
    "    try:\n"
    "        result = func(*args)\n"
    "    except Exception as e:\n"
    "        result = e\n"
    "    finally:\n"
    "        _ti._eval_set_result(pid, result)\n"
    "\n"
    "def _ti_call_ext(pid, func, *args):\n"
    "    t = threading.Thread(\n"
    "        target=_ti_work,\n"
    "        args=(pid, func) + tuple(args))\n"
    "    t.start()\n";


PyObject * ext_py__val_to_py(ti_val_t * val, ex_t * e)
{
    switch((ti_val_enum) val->tp)
    {
    case TI_VAL_NIL:
        Py_RETURN_NONE;
    case TI_VAL_CLOSURE:
        ex_set(e, EX_TYPE_ERROR,
                "cannot convert type `"TI_VAL_CLOSURE_S"` to Python");
        return NULL;
    case TI_VAL_INT:
    {
        PyObject * val = PyLong_FromLongLong(VINT(val));
        if (!val)
            goto mem_error;
        return val;
    }
    case TI_VAL_FLOAT:
    {
        PyObject * val = PyLong_FromDouble(VFLOAT(val));
        if (!val)
            goto mem_error;
        return val;
    }
    case TI_VAL_BOOL:
        return VBOOL(val)
                ? Py_INCREF(Py_True), Py_True
                : Py_INCREF(Py_False), Py_False;
    case TI_VAL_DATETIME:
    case TI_VAL_NAME:
    case TI_VAL_STR:
    case TI_VAL_BYTES:
    case TI_VAL_REGEX:
    case TI_VAL_THING:
    case TI_VAL_WRAP:
    case TI_VAL_ARR:
    case TI_VAL_SET:
    case TI_VAL_ERROR:
    case TI_VAL_MEMBER:
    case TI_VAL_MP:
    case TI_VAL_FUTURE:
    case TI_VAL_TEMPLATE:
        break;
    }

mem_error:
    ex_mem_set(e);
    return NULL;
}


/*
 * Create set_result function
 */
static PyObject * ti_set_result(PyObject * UNUSED(self), PyObject * args)
{
    ex_t e = {0};
    ti_future_t * future;
    PyObject * pRes, * pNone;
    intptr_t ptr;
    ti_future_t * chk;
    size_t idx = 0;

    if (!PyArg_ParseTuple(args, "LO", &ptr, &pRes))
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


int ti_ext_py_init(void)
{
    if (PyImport_AppendInittab("_ti", &PyInit_ti) < 0)
    {
        log_error("(py) failed to load _ti module");
        return -1;
    }

    Py_Initialize();

    if (ext_py__insert_module_path)
        goto fail0;

    if (PyRun_SimpleString(ext_py__init_code))
    {
        log_error("(py) failed to run initial code");
    }

    main_module = PyImport_ImportModule("__main__");
    pName = PyUnicode_DecodeFSDefaultAndSize("request", 7);
    /* Error checking of pName left out */

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);


fail0:
    (void) Py_FinalizeEx();
    return -1;
}

static int ext_py__run_thread(void)
{

}


static void ext_py__work(uv_work_t * UNUSED(work))
{
    PyObject * pName, * pModule, * pFunc, * pFunc2, * pFunc3, *main_module, *sys_module, * Ppath;
    PyObject *pArgs, *pValue;
//        PyObject *pName, *pModule, *pDict,
//                   *pFunc, *pInstance, *pArgs, *pValue;
//        PyThreadState *mainThreadState, *myThreadState, *tempState;
//        PyInterpreterState *mainInterpreterState;
    PyImport_AppendInittab("_ti", &PyInit_ti);
    Py_Initialize();

    sys_module = PyImport_ImportModule("sys");
    if (!sys_module)
    {
        LOGC("ERROR");
    }
    Ppath = PyObject_GetAttrString(sys_module, "path");
    pFunc3 = PyObject_GetAttrString(Ppath, "insert");

    pArgs = PyTuple_New(2);
    pValue = PyLong_FromLong(0);
    pName = PyUnicode_DecodeFSDefault("/home/joente/workspace/thingsdb/src/ti/ext");

    /* pValue reference stolen here: */
    PyTuple_SetItem(pArgs, 0, pValue);
    PyTuple_SetItem(pArgs, 1, pName);
    pValue = PyObject_CallObject(pFunc3, pArgs);
    Py_DECREF(pValue);

    PyRun_SimpleString(pycode);

    main_module = PyImport_ImportModule("__main__");
    pName = PyUnicode_DecodeFSDefaultAndSize("request", 7);
    /* Error checking of pName left out */

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);


    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(main_module, "_ti_call_ext");
        pFunc2 = PyObject_GetAttrString(pModule, "main");
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            pArgs = PyTuple_New(2);
            pValue = PyLong_FromLong(1234);
            /* pValue reference stolen here: */
            PyTuple_SetItem(pArgs, 0, pValue);
            PyTuple_SetItem(pArgs, 1, pFunc2);

            pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                Py_DECREF(pValue);
            }
            else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                fprintf(stderr,"Call failed\n");
            }
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function `main`\n");
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"request\"\n");
    }

    (void) Py_FinalizeEx();


//        PyGILState_STATE state;
//        PyThreadState * tstate;
//
//        // Random testing code
//        for(i = 0; i < 3; i++)
//        {
//            printf("...Printed from my thread.\n");
//            sleep(1);
//        }
//
//        if (!gil_init) {
//            gil_init = 1;
//            PyEval_InitThreads();
//                    tstate = PyEval_SaveThread();
//        }
////        tstate = PyEval_SaveThread();
//        state = PyGILState_Ensure();
////        PyEval_RestoreThread(tstate);
//        PyGILState_Release(state);

        // Initialize python inerpreter
//        Py_Initialize();
//
//        // Initialize thread support
//        PyEval_InitThreads();

        // Save a pointer to the main PyThreadState object
//        mainThreadState = PyThreadState_Get();
//
//        // Get a reference to the PyInterpreterState
//        mainInterpreterState = mainThreadState->interp;
//
//        // Create a thread state object for this thread
//        myThreadState = PyThreadState_New(mainInterpreterState);
//
//        // Release global lock
////        PyEval_ReleaseLock();
//
//        // Acquire global lock
//        PyEval_AcquireLock();
//
//        // Swap in my thread state
//        tempState = PyThreadState_Swap(myThreadState);

//        // Now execute some python code (call python functions)
//        pName = PyString_FromString(arg->argv[1]);
//        pModule = PyImport_Import(pName);
//
//        // pDict and pFunc are borrowed references
//        pDict = PyModule_GetDict(pModule);
//        pFunc = PyDict_GetItemString(pDict, arg->argv[2]);
//
//        if (PyCallable_Check(pFunc))
//        {
//            pValue = PyObject_CallObject(pFunc, NULL);
//        }
//        else {
//            PyErr_Print();
//        }
//
//        // Clean up
//        Py_DECREF(pModule);
//        Py_DECREF(pName);

//        // Swap out the current thread
//        PyThreadState_Swap(tempState);
//
//        // Release global lock
//        PyEval_ReleaseLock();
//
//        // Clean up thread state
//        PyThreadState_Clear(myThreadState);
//        PyThreadState_Delete(myThreadState);

//        PyEval_SaveThread();
//
        printf("My thread is finishing...\n");
}

static void ext_py__work_finish(uv_work_t * work, int status)
{
    ex_t e = {0};
    ti_future_t * future = work->data;

    future->rval = (ti_val_t *) ti_varr_create(0);
    fprintf(stderr, "status: %d\n\n", status);

    ti_query_on_future_result(future, &e);
    free(work);
//    uv_close((uv_handle_t *) work, (uv_close_cb) free);
}

void ti_ext_py_cb(ti_future_t * future)
{
    if (vec_push(&ti.futures, future))
    {
        ex_t e;
        ex_set_mem(&e);
        ti_query_on_future_result(future, &e);
        return;
    }

    if (uv_queue_work(
            ti.loop,
            work,
            ext_py__work,
            ext_py__work_finish))
    {
        ex_t e;
        ex_set_internal(&e);
        ti_query_on_future_result(future, &e);
    }
}

