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

Notes:
 - All commands are run as if in a single interpreter session. Imports and
   function definitions persist.
 - Don't use the eval subcommand with substituted input.

	py eval evalString

example script:

	py eval {def mk(dir): os.mkdir(dir)}
	py eval {import os; mk('testdir')}

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

1. `py import ?-from module? module : -> nil`
2. `py call func ?arg ...? : ?str ...? -> str` (str args, str return)
3. `py call func ?arg ...? : ?str ...? -> multi` (str arg, polymorphic return)
4. `py call -types [list t1 ...] func ?arg ...? : ?t1 ...? -> multi`
   (polymorphic args, polymorphic return)
5. allow statically compiling python into tclpy
6. allow statically compiling tclpy
