#include <Python.h>
#include <tcl.h>
#include <assert.h>
#include <dlfcn.h>

// Need an integer we can use for detecting python errors, assume we'll never
// use TCL_BREAK
#define PY_ERROR TCL_BREAK

static PyObject *pFormatException = NULL;
static PyObject *pFormatExceptionOnly = NULL;

// Returns a string that must be 'free'd containing an error and traceback, or
// NULL if there was no Python error
static char *
pyTraceAsStr(void)
{
	// Shouldn't call this function unless Python has excepted
	if (PyErr_Occurred() == NULL)
		return NULL;

	/* USE PYTHON TRACEBACK MODULE */

	// TODO: save the error and reraise in python if we have no idea
	// TODO: prefix everything with 'PY:'?
	// TODO: use extract_tb to get stack, print custom traceback myself

	PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;
	PyObject *pTraceList = NULL, *pTraceStr = NULL, *pTraceDesc = NULL;
	PyObject *pNone = NULL, *pEmptyStr = NULL;
	char *traceStr = NULL;
	Py_ssize_t traceLen = 0;

	PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
	PyErr_NormalizeException(&pType, &pVal, &pTrace);

	/* Get traceback as a python list */
	if (pTrace != NULL) {
		pTraceList = PyObject_CallFunctionObjArgs(
			pFormatException, pType, pVal, pTrace, NULL);
	} else {
		pTraceList = PyObject_CallFunctionObjArgs(
			pFormatExceptionOnly, pType, pVal, NULL);
	}
	if (pTraceList == NULL)
		return strdup("[Failed to get python exception details (#e_ltp01)]\n");

	/* Put the list in tcl order (top stack level at top) */
	pNone = PyObject_CallMethod(pTraceList, "reverse", NULL);
	if (pNone == NULL) {
		Py_DECREF(pTraceList);
		return strdup("[Failed to get python exception details (#e_ltp02)]\n");
	}
	assert(pNone == Py_None);
	Py_DECREF(pNone);

	/* Remove "Traceback (most recent call last):" if the trace len > 1 */
	/* TODO: this feels like a hack, there must be a better way */
	traceLen = PyObject_Length(pTraceList);
	if (traceLen > 1) {
		pTraceDesc = PyObject_CallMethod(pTraceList, "pop", NULL);
	}
	if (traceLen <= 0 || (pTraceDesc == NULL && traceLen > 1)) {
		Py_DECREF(pTraceList);
		return strdup("[Failed to get python exception details (#e_ltp03)]\n");
	}
	Py_XDECREF(pTraceDesc);

	/* Turn the python list into a python string */
	pEmptyStr = PyString_FromString("");
	if (pEmptyStr == NULL) {
		Py_DECREF(pTraceList);
		return strdup("[Failed to get python exception details (#e_ltp04)]\n");
	}
	pTraceStr = PyObject_CallMethod(pEmptyStr, "join", "O", pTraceList);
	Py_DECREF(pTraceList);
	Py_DECREF(pEmptyStr);
	if (pTraceStr == NULL)
		return strdup("[Failed to get python exception details (#e_ltp05)]\n");

	/* Turn the python string into a string */
	traceStr = strdup(PyString_AS_STRING(pTraceStr));
	Py_DECREF(pTraceStr);

	return traceStr;
}

static int
PyCall_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "func ?arg ...?");
		return TCL_ERROR;
	}

	const char *objandfn = Tcl_GetString(objv[2]);

	/* Borrowed ref, do not decrement */
	PyObject *pMainModule = PyImport_AddModule("__main__");
	if (pMainModule == NULL)
		return PY_ERROR;

	/* So we don't have to special case the decref in the following loop */
	Py_INCREF(pMainModule);
	PyObject *pObjParent = NULL;
	PyObject *pObj = pMainModule;
	PyObject *pObjStr = NULL;
	char *dot = index(objandfn, '.');
	while (dot != NULL) {
		pObjParent = pObj;

		pObjStr = PyString_FromStringAndSize(objandfn, dot-objandfn);
		if (pObjStr == NULL) {
			Py_DECREF(pObjParent);
			return PY_ERROR;
		}

		pObj = PyObject_GetAttr(pObjParent, pObjStr);
		Py_DECREF(pObjStr);
		Py_DECREF(pObjParent);
		if (pObj == NULL)
			return PY_ERROR;

		objandfn = dot + 1;
		dot = index(objandfn, '.');
	}

	PyObject *pFn = PyObject_GetAttrString(pObj, objandfn);
	Py_DECREF(pObj);
	if (pFn == NULL)
		return PY_ERROR;

	if (!PyCallable_Check(pFn)) {
		Py_DECREF(pFn);
		return PY_ERROR;
	}

	int i;
	PyObject *pArgs = PyTuple_New(objc-3);
	PyObject* curarg = NULL;
	for (i = 0; i < objc-3; i++) {
		curarg = PyString_FromString(Tcl_GetString(objv[i+3]));
		if (curarg == NULL) {
			Py_DECREF(pArgs);
			Py_DECREF(pFn);
			return PY_ERROR;
		}
		/* Steals a reference */
		PyTuple_SET_ITEM(pArgs, i, curarg);
	}

	PyObject *pRet = PyObject_Call(pFn, pArgs, NULL);
	Py_DECREF(pFn);
	Py_DECREF(pArgs);
	if (pRet == NULL)
		return PY_ERROR;

	PyObject *pStrRet = PyObject_Str(pRet);
	Py_DECREF(pRet);
	if (pStrRet == NULL)
		return PY_ERROR;

	char *ret = PyString_AS_STRING(pStrRet);
	Py_DECREF(pStrRet);
	if (ret == NULL)
		return PY_ERROR;

	Tcl_SetResult(interp, ret, TCL_VOLATILE);
	return TCL_OK;
}

static int
PyEval_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "evalString");
		return TCL_ERROR;
	}

	const char *cmd = Tcl_GetString(objv[2]);

	if (PyRun_SimpleString(cmd) == 0) {
		return TCL_OK;
	} else {
		return PY_ERROR;
	};
}

static int
PyImport_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	char *modname, *topmodname;
	PyObject *pMainModule, *pTopModule;
	int ret = -1;

	if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "module");
		return TCL_ERROR;
	}

	modname = Tcl_GetString(objv[2]);

	/* Borrowed ref, do not decrement */
	pMainModule = PyImport_AddModule("__main__");
	if (pMainModule == NULL)
		return PY_ERROR;

	pTopModule = PyImport_ImportModuleEx(modname, NULL, NULL, NULL);
	if (pTopModule == NULL)
		return PY_ERROR;

	topmodname = PyModule_GetName(pTopModule);
	if (topmodname != NULL) {
		ret = PyObject_SetAttrString(pMainModule, topmodname, pTopModule);
	}
	Py_DECREF(pTopModule);

	if (ret != -1) {
		return TCL_OK;
	} else {
		return PY_ERROR;
	}
}

/* The two static variables below are related by order, keep alphabetical */
static const char *cmdnames[] = {
	"call", "eval", "import", NULL
};
static int (*cmds[]) (
	ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]
) = {
	PyCall_Cmd, PyEval_Cmd, PyImport_Cmd
};

static int
Py_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg ...?");
		return TCL_ERROR;
	}

	int cmdindex;
	if (Tcl_GetIndexFromObj(interp, objv[1], cmdnames, "command", TCL_EXACT,
			&cmdindex) != TCL_OK)
		return TCL_ERROR;

	/* Actually call the command */
	int ret = (*(cmds[cmdindex]))(clientData, interp, objc, objv);

	if (ret == PY_ERROR) {
		ret = TCL_ERROR;
		// Not entirely sure if this is the correct way of doing things. Should
		// I be calling Tcl_AddErrorInfo instead?
		char *traceStr = pyTraceAsStr();
		Tcl_AppendResult(interp, traceStr, NULL);
		Tcl_AppendResult(interp, "----- tcl -> python interface -----", NULL);
		free(traceStr);
		/* In case there was an exception in the traceback printer */
		PyErr_Clear();
	}

	return ret;
}

int
Tclpy_Init(Tcl_Interp *interp)
{
	if (Tcl_InitStubs(interp, "8.5", 0) == NULL)
		return TCL_ERROR;
	if (Tcl_PkgRequire(interp, "Tcl", "8.5", 0) == NULL)
		return TCL_ERROR;
	if (Tcl_PkgProvide(interp, "tclpy", PACKAGE_VERSION) != TCL_OK)
		return TCL_ERROR;

	Tcl_Command cmd = Tcl_CreateObjCommand(interp, "py",
		(Tcl_ObjCmdProc *) Py_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	if (cmd == NULL)
		return TCL_ERROR;

	/* Hack to fix Python C extensions not linking to libpython*.so */
	/* http://bugs.python.org/issue4434 */
	dlopen(PY_LIBFILE, RTLD_LAZY | RTLD_GLOBAL);

	Py_Initialize(); /* void */

	/* Get support for full tracebacks */
	PyObject *pTraceModStr, *pTraceMod;

	pTraceModStr = PyString_FromString("traceback");
	if (pTraceModStr == NULL)
		return TCL_ERROR;
	pTraceMod = PyImport_Import(pTraceModStr);
	Py_DECREF(pTraceModStr);
	if (pTraceMod == NULL)
		return TCL_ERROR;
	pFormatException =     PyObject_GetAttrString(pTraceMod, "format_exception");
	pFormatExceptionOnly = PyObject_GetAttrString(pTraceMod, "format_exception_only");
	Py_DECREF(pTraceMod);
	if (pFormatException == NULL || pFormatExceptionOnly == NULL ||
			!PyCallable_Check(pFormatException) ||
			!PyCallable_Check(pFormatExceptionOnly)) {
		Py_XDECREF(pFormatException);
		Py_XDECREF(pFormatExceptionOnly);
		return TCL_ERROR;
	}

	return TCL_OK;
}
