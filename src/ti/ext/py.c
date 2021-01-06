/*
 * ti/ext/async.c
 */
#define PY_SSIZE_T_CLEAN
#include <ti/ext/async.h>
#include <ti/varr.h>
#include <ti/nil.h>
#include <ti/query.h>
#include <ti.h>
#include <Python.h>


/* Return the number of arguments of the application command line */
static PyObject * ti_set_result(PyObject * UNUSED(self), PyObject * args)
{
    PyObject * pRes;
    intptr_t i;
    if (!PyArg_ParseTuple(args, "LO", &i, &pRes))
    {
        printf("Failed\n");
        return NULL;
    }

    printf("Result of module (%ld): %ld\n", i, PyLong_AsLong(pRes));
    Py_INCREF(pRes);
    return pRes;
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

const char * pycode =
    "import threading\n"
    "import _ti\n"
    "\n"
    "class _TiCall:\n"
    "    def __init__(self, pid):\n"
    "        self.pid = pid\n"
    "    \n"
    "    def set_result(self, result):\n"
    "        if self.pid is None:\n"
    "            return\n"
    "        pid = self.pid\n"
    "        self.pid = None\n"
    "        _ti.set_result(pid, result)\n"
    "\n"
    "\n"
    "def _ti_work(pid, func, *args):\n"
    "    ti_call = _TiCall(pid)\n"
    "    try:\n"
    "        func(ti_call.set_result, *args)\n"
    "    except Exception as e:\n"
    "        ti_call.set_result(e)\n"
    "    finally:\n"
    "        ti_call.set_result(None)\n"
    "\n"
    "\n"
    "def _ti_call_ext(pid, func, *args):\n"
    "    t = threading.Thread(\n"
    "        target=_ti_work,\n"
    "        args=(pid, func) + tuple(args))\n"
    "    t.start()\n";

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
    uv_work_t * work = malloc(sizeof(uv_work_t));
    if (!work)
    {
        ex_t e;
        ex_set_mem(&e);
        ti_query_on_future_result(future, &e);
    }

    work->data = future;

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

