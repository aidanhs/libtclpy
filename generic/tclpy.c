#include <Python.h>
#include <tcl.h>
#include <assert.h>

// Returns a string that must be 'free'd containing an error and traceback, or
// NULL if there was no Python error
static char *
pyTraceAsStr(void)
{
	// Shouldn't call this function unless Python has excepted
	if (PyErr_Occurred() == NULL)
		return NULL;

	/* pTrace unused */
	PyObject *pType = NULL, *pVal = NULL, *pTrace = NULL;
	PyObject *pRet = NULL;

	PyErr_Fetch(&pType, &pVal, &pTrace); /* Clears exception */
	PyErr_NormalizeException(&pType, &pVal, &pTrace);

	/* Only have the type (PyErr_SetNone has possibly been called) */
	if (pVal == NULL) {
		pRet = pType;
	} else {
	/* Basic error string */
		pRet = pVal;
	}
	PyObject *pRetStr = PyObject_Str(pRet);
	if (pRetStr == NULL)
		return strdup("[failed to get Python error value]");
	char *retStr = strdup(PyString_AS_STRING(pRetStr));
	Py_DECREF(pRetStr);
	return retStr;
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
		return TCL_ERROR;

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
			return TCL_ERROR;
		}

		pObj = PyObject_GetAttr(pObjParent, pObjStr);
		Py_DECREF(pObjStr);
		Py_DECREF(pObjParent);
		if (pObj == NULL)
			return TCL_ERROR;

		objandfn = dot + 1;
		dot = index(objandfn, '.');
	}

	PyObject *pFn = PyObject_GetAttrString(pObj, objandfn);
	Py_DECREF(pObj);
	if (pFn == NULL)
		return TCL_ERROR;

	if (!PyCallable_Check(pFn)) {
		Py_DECREF(pFn);
		return TCL_ERROR;
	}

	int i;
	PyObject *pArgs = PyTuple_New(objc-3);
	PyObject* curarg = NULL;
	for (i = 0; i < objc-3; i++) {
		curarg = PyString_FromString(Tcl_GetString(objv[i+3]));
		if (curarg == NULL) {
			Py_DECREF(pArgs);
			Py_DECREF(pFn);
			return TCL_ERROR;
		}
		/* Steals a reference */
		PyTuple_SET_ITEM(pArgs, i, curarg);
	}

	PyObject *pRet = PyObject_Call(pFn, pArgs, NULL);
	Py_DECREF(pFn);
	Py_DECREF(pArgs);
	if (pRet == NULL)
		return TCL_ERROR;

	PyObject *pStrRet = PyObject_Str(pRet);
	Py_DECREF(pRet);
	if (pStrRet == NULL)
		return TCL_ERROR;

	char *ret = PyString_AS_STRING(pStrRet);
	Py_DECREF(pStrRet);
	if (ret == NULL)
		return TCL_ERROR;

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
		return TCL_ERROR;
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
		return TCL_ERROR;

	pTopModule = PyImport_ImportModuleEx(modname, NULL, NULL, NULL);
	if (pTopModule == NULL)
		return TCL_ERROR;

	topmodname = PyModule_GetName(pTopModule);
	if (topmodname != NULL) {
		ret = PyObject_SetAttrString(pMainModule, topmodname, pTopModule);
	}
	Py_DECREF(pTopModule);

	if (ret != -1) {
		return TCL_OK;
	} else {
		return TCL_ERROR;
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

	if (ret == TCL_ERROR) {
		char *traceStr = pyTraceAsStr();
		Tcl_AppendResult(interp, traceStr, NULL);
		free(traceStr);
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
	if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK)
		return TCL_ERROR;

	Tcl_Command cmd = Tcl_CreateObjCommand(interp, "py",
		(Tcl_ObjCmdProc *) Py_Cmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	if (cmd == NULL)
		return TCL_ERROR;

	Py_Initialize();

	return TCL_OK;
}
