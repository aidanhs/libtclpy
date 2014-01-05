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

static const char *cmdnames[] = {
	"eval", NULL
};
static int (*cmds[]) (
	ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]
) = {
	PyEval_Cmd
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
		Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?argument ...?");
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

	Tcl_CreateObjCommand(interp, "py", (Tcl_ObjCmdProc *) Py_Cmd,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	Py_Initialize();

	return TCL_OK;
}
