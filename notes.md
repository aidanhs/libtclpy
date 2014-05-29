http://www.tcl.tk/man/tcl/TclLib/contents.htm

tclsh <(echo -e "load libtclpy0.1.so\npy import hashlib}\nx")
crashes badly

# Exceptions

Alternative approaches
```
	// http://stackoverflow.com/questions/16733425/how-to-retrieve-filename-and-lineno-attribute-of-syntaxerror
	// http://stackoverflow.com/questions/1796510/accessing-a-python-traceback-from-the-c-api
	// http://www.gossamer-threads.com/lists/python/python/150924
	// https://mail.python.org/pipermail/capi-sig/2009-January/000197.html
	// http://docs.python.org/2/library/traceback.html

#if 0
	/* WALK FRAMES BACKWARDS */

	PyThreadState *tstate = PyThreadState_GET();
	if (tstate != NULL && tstate->frame != NULL) {
		PyFrameObject *frame = tstate->frame;

		printf("Python stack trace:\n");
		while (frame != NULL) {
			int line = frame->f_lineno;
			const char *filename = PyString_AsString(frame->f_code->co_filename);
			const char *funcname = PyString_AsString(frame->f_code->co_name);
			printf("    %s(%d): %s\n", filename, line, funcname);
			frame = frame->f_back;
		}
	}
#endif

#if 0
	/* USE EXCEPTION STRUCT */

	PyObject *pType, *pVal, *pTrace;

	PyErr_Fetch(&pType, &pVal, &pTrace);
	PyErr_NormalizeException(&pType, &pVal, &pTrace);

	PyTracebackObject* traceback = (PyTracebackObject *)pTrace;
	// Advance to the last frame (python puts the most-recent call at the end)
	while (traceback->tb_next != NULL)
		traceback = traceback->tb_next;

	int line = traceback->tb_lineno;
	const char* filename = PyString_AsString(traceback->tb_frame->f_code->co_filename)
#endif
```

