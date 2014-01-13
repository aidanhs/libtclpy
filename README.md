tclpy
=====

This is a tcl extension for interacting with Python, targeting tcl >= 8.4.

The extension itself (files: generic/\*, tests/tclpy.test) is available
under the 3-clause BSD license (see "LICENSE").

The tools for building and testing the extension (files: configure.in,
Makefile.in, tclconfig/\*, tests/all.tcl, win/\*) are distributed under the terms
in "LICENSE.tea".

USAGE
-----

General notes`:
 - Unless otherwise noted, 'interpreter' refers to the python interpreter.
 - All commands are run in the context of a single interpreter session. Imports,
   function definitions and variables persist.

Reference:
 - `py eval evalString`
   - `takes: string of valid python code`
   - `returns: nothing`
   - `side effects: executes code in the python interpreter`
   - **Do not use with substituted input**
   - `evalString` may be any valid python code, including semicolons for single
     line statements or (non-indented) multiline blocks
 - `py import module`
   - `takes: name of a python module`
   - `returns: nothing`
   - `side effects: imports named module into globals of the python interpreter`
   - the name of the module may be of the form module.submodule

example script:

	py eval {def mk(dir): os.mkdir(dir)}
	py import os
	py eval {print "creating 'testdir'"; mk('testdir')}

UNIX BUILD
----------

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see
the tcl/unix/README file in the Tcl src dist. The following minimal
example will install the extension in the /opt/tcl directory.

	$ cd sampleextension
	$ ./configure --prefix=/opt/tcl
	$ make
	$ make install

WINDOWS BUILD
-------------

The tclpy Windows build is untested - the instructions below are for
generic TEA extensions and may or may not work.

The recommended method to build extensions under windows is to use the
Msys + Mingw build process. This provides a Unix-style build while
generating native Windows binaries. Using the Msys + Mingw build tools
means that you can use the same configure script as per the Unix build
to create a Makefile. See the tcl/win/README file for the URL of
the Msys + Mingw download.

If you have VC++ then you may wish to use the files in the win
subdirectory and build the extension using just VC++. These files have
been designed to be as generic as possible but will require some
additional maintenance by the project developer to synchronise with
the TEA configure.in and Makefile.in files. Instructions for using the
VC++ makefile are written in the first part of the Makefile.vc
file.

TODO
----

In order of priority:

 - `py call func ?arg ...? : ?str ...? -> str` (str args, str return)
 - `py call func ?arg ...? : ?str ...? -> multi` (str arg, polymorphic return)
 - `py call -types [list t1 ...] func ?arg ...? : ?t1 ...? -> multi`
 - (polymorphic args, polymorphic return)
 - allow statically compiling python into tclpy
   - http://pkaudio.blogspot.co.uk/2008/11/notes-for-embedding-python-in-your-cc.html
   - https://github.com/albertz/python-embedded
   - https://github.com/zeha/python-superstatic
   - http://www.velocityreviews.com/forums/t741756-embedded-python-static-modules.html
   - http://christian.hofstaedtler.name/blog/2013/01/embedding-python-on-win32.html
   - http://stackoverflow.com/questions/1150373/compile-the-python-interpreter-statically
 - allow statically compiling tclpy
 - `py import ?-from module? module : -> nil`
