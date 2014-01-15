#include <Python.h>
#include <tcl.h>

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

	const char *fn = Tcl_GetString(objv[2]);

	// Borrowed ref, do not decrement
	PyObject *pMainModule = PyImport_AddModule("__main__");
	if (pMainModule == NULL)
		return TCL_ERROR;

	PyObject *pFn = PyObject_GetAttrString(pMainModule, fn);
	if (pFn == NULL)
		return TCL_ERROR;

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
		PyTuple_SET_ITEM(pArgs, i, curarg);
	}

	PyObject *pRet = PyObject_Call(pFn, pArgs, NULL);
	Py_DECREF(pFn);
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

	// Borrowed ref, do not decrement
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

	return (*(cmds[cmdindex]))(clientData, interp, objc, objv);
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
