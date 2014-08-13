tclpy
=====

This is a tcl extension for interacting with Python, targeting tcl >= 8.5.

The extension is available under the 3-clause BSD license (see "LICENSE").

USAGE
-----

General notes:
 - Unless otherwise noted, 'interpreter' refers to the python interpreter.
 - All commands are run in the context of a single interpreter session. Imports,
   function definitions and variables persist.
 - Exceptions in the python interpreter will return a stack trace of the python
   code that was executing. If the exception continues up the stack, the tcl
   stack trace will be appended to it.
   They may be masked (as per tcl stack traces) with catch.

Reference:
 - `py call ?obj.?func ?arg ...?`
   - `takes: name of a python function`
   - `returns: return value of function with str() applied to it, except:`
     - `True is converted to 1`
     - `False is converted to 0`
     - `None is converted to an empty string`
     - `Python 'str' objects are considered to be byte arrays`
     - `Python 'unicode' objects are considered to be unicode strings`
   - `side effects: executes function`
   - `func` may be a dot qualified name (i.e. object or module method)
 - `py eval evalString`
   - `takes: string of valid python code`
   - `returns: nothing`
   - `side effects: executes code in the python interpreter`
   - **Do not use with substituted input**
   - `evalString` may be any valid python code, including semicolons for single
     line statements or (non-indented) multiline blocks
   - errors reaching the python interpreter top level are printed to stderr
 - `py import module`
   - `takes: name of a python module`
   - `returns: nothing`
   - `side effects: imports named module into globals of the python interpreter`
   - the name of the module may be of the form module.submodule

example tclsh session:

```
% load libtclpy.so
%
% py eval {def mk(dir): os.mkdir(dir)}
% py eval {def rm(dir): os.rmdir(dir); return 15}
% py import os
% set a [py eval {print "creating 'testdir'"; mk('testdir')}]
creating 'testdir'
% set b [py call rm testdir]
15
%
% py import StringIO
% py eval {sio = StringIO.StringIO()}
% py call sio.write someinput
None
% set c [py call sio.getvalue]
someinput
%
% py eval {divide = lambda x: 1.0/int(x)}
% set d [py call divide 16]
0.0625
% list [catch {py call divide 0} err] $err
1 {ZeroDivisionError: float division by zero
  File "<string>", line 1, in <lambda>
----- tcl -> python interface -----}
%
% py import json
% py eval {
def jobj(*args):
    d = {}
    for i in range(len(args)/2):
        d[args[2*i]] = args[2*i+1]
    return json.dumps(d)
}
% set e [dict create]
% dict set e {t"est} "11{24"
t\"est 11\{24
% dict set e 6 5
t\"est 11\{24 6 5
% set e [py call jobj {*}$e]
{"t\"est": "11{24", "6": "5"}
% puts "a: $a, b: $b, c: $c, d: $d, e: $e"
a: , b: 15, c: someinput, d: 0.0625, e: {"t\"est": "11{24", "6": "5"}
```

UNIX BUILD
----------

The build process is very simple. Make sure you can run python-config and have
a tclConfig.sh file somewhere. For Ubuntu the process would be:

	$ sudo apt-get install python-dev tcl-dev
	$ make

For other distros you would need give the path of tclConfig.sh:

	$ make TCLCONFIG=/usr/lib/tclConfig.sh

TESTS
-----

Run the tests with

	$ make test

GOTCHAS
-------

1. Be very careful when putting unicode characters into a inside a `py eval`
call - they are decoded by the tcl parser and passed as literal bytes
to the python interpreter. So if we directly have the character "ಠ", it is
decoded to a utf-8 byte sequence and becomes u"\xe0\xb2\xa0" (where the \xXY are
literal bytes) as seen by the Python interpreter.
2. Escape sequences (e.g. `\x00`) inside py eval may be interpreted by tcl - use
{} quoting to avoid this.

TODO
----

In order of priority:

 - `py call ?mod.?func ?arg ...? : ?str ...? -> multi` (str arg, polymorphic return)
 - `py call -types [list t1 ...] func ?arg ...? : ?t1 ...? -> multi`
   (polymorphic args, polymorphic return)
 - unicode handling (in exception messages, returns from calls...AS\_STRING is bad)
 - allow statically compiling python into tclpy
   - http://pkaudio.blogspot.co.uk/2008/11/notes-for-embedding-python-in-your-cc.html
   - https://github.com/albertz/python-embedded
   - https://github.com/zeha/python-superstatic
   - http://www.velocityreviews.com/forums/t741756-embedded-python-static-modules.html
   - http://christian.hofstaedtler.name/blog/2013/01/embedding-python-on-win32.html
   - http://stackoverflow.com/questions/1150373/compile-the-python-interpreter-statically
 - allow python to call back into tcl
 - allow statically compiling tclpy
 - let `py eval` work with indented multiline blocks
 - `py import ?-from module? module : -> nil`
 - return the short error line in the catch err variable and put the full stack
   trace in errorInfo
 - py call of non-existing function says raises attribute err, should be a
   NameError
 - make `py call` look in the builtins module - http://stackoverflow.com/a/11181607
 - all TODOs
