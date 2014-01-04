#include <Python.h>
#include <tcl.h>

static int
PyEval_Cmd(
	ClientData clientData,  /* Not used. */
	Tcl_Interp *interp,     /* Current interpreter */
	int objc,               /* Number of arguments */
	Tcl_Obj *const objv[]   /* Argument strings */
	)
{
	if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "evalString");
		return TCL_ERROR;
	}

	const char *cmd = Tcl_GetString(objv[1]);

	if (PyRun_SimpleString(cmd) == 0) {
		return TCL_OK;
	} else {
		return TCL_ERROR;
	};
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

	Tcl_CreateObjCommand(interp, "pyEval", (Tcl_ObjCmdProc *) PyEval_Cmd,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	Py_Initialize();

	return TCL_OK;
}
