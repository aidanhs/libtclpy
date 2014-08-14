PACKAGE_VERSION = 0.3

DFLAGS = -DPACKAGE_VERSION='"$(PACKAGE_VERSION)"'

# Flags to improve security
CFLAGS_SEC = \
	-fstack-protector \
	--param=ssp-buffer-size=4 \
	-Wformat \
	-Werror=format-security \
	-D_FORTIFY_SOURCE=2\
	-Wl,-z,relro,-z,now
# Protect against my own poor programming
CFLAGS_SAFE = -fno-strict-overflow
# Tell me when I'm doing something wrong
CFLAGS_WARN = \
	-Wall -Wextra \
	-Wstrict-aliasing -Wstrict-overflow -Wstrict-prototypes
# Not interested in these warnings
CFLAGS_NOWARN = -Wno-unused-parameter
# Speed things up
CFLAGS_FAST = -O2

CFLAGS = \
	$(CFLAGS_SEC) $(CFLAGS_SAFE) \
	$(CFLAGS_WARN) $(CFLAGS_NOWARN) \
	$(CFLAGS_FAST)

# TODO:
#  - check python-config works
#  - check provided tclConfig.sh exists
#  - check stubs are supported (TCL_SUPPORTS_STUBS)

TCLCONFIG?=/usr/lib/tclConfig.sh
TCL_LIB     = $(shell . $(TCLCONFIG); echo $$TCL_STUB_LIB_SPEC) -DUSE_TCL_STUBS
TCL_INCLUDE = $(shell . $(TCLCONFIG); echo $$TCL_INCLUDE_SPEC)
PY_LIB      = $(shell python-config --libs)
PY_INCLUDE  = $(shell python-config --includes)

PY_LIBFILE  = $(shell python -c 'import distutils.sysconfig; print distutils.sysconfig.get_config_var("LDLIBRARY")')
CFLAGS += -DPY_LIBFILE='"$(PY_LIBFILE)"'


default: libtclpy$(PACKAGE_VERSION).so


libtclpy$(PACKAGE_VERSION).so: tclpy.o pkgIndex.tcl
	rm -f libtclpy.so
	gcc -shared -fPIC $(CFLAGS) $< -o $@ -Wl,--export-dynamic $(TCL_LIB) $(PY_LIB)
	ln -s $@ libtclpy.so

tclpy.o: generic/tclpy.c
	test -f $(TCLCONFIG)
	gcc -fPIC $(CFLAGS) $(DFLAGS) \
		$(PY_INCLUDE) $(TCL_INCLUDE) -c ./generic/tclpy.c -o tclpy.o

pkgIndex.tcl: pkgIndex.tcl.in
	sed "s/PACKAGE_VERSION/$(PACKAGE_VERSION)/g" pkgIndex.tcl.in > pkgIndex.tcl

clean:
	rm -f *.so *.o pkgIndex.tcl

test: default
	TCLLIBPATH=. tclsh tests/tclpy.test

.PHONY: clean test default
