PACKAGE_VERSION = 0.4

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
#  - check stubs are supported (TCL_SUPPORTS_STUBS)

TCL_STUBS ?= 1
TCLCONFIG ?= $(shell \
	(X=/usr/lib/tcl8.6/tclConfig.sh; test -f $$X && echo $$X || exit 1) || \
	(X=/usr/lib64/tclConfig.sh; test -f $$X && echo $$X || exit 1) || \
	(X=/usr/lib/tclConfig.sh; test -f $$X && echo $$X || exit 1) || \
	echo "" \
)
TCLCONFIG_TEST = test -f "$(TCLCONFIG)" || (echo "Couldn't find tclConfig.sh" && exit 1)

TCL_LIB     = $(shell . "$(TCLCONFIG)"; \
	if [ "$(TCL_STUBS)" = 1 ]; then \
		echo "$$TCL_STUB_LIB_SPEC -DUSE_TCL_STUBS"; \
	else \
		echo "$$TCL_LIB_SPEC"; \
	fi \
)
TCL_INCLUDE = $(shell . "$(TCLCONFIG)"; echo $$TCL_INCLUDE_SPEC)
PY_LIB      = $(shell python3-config --libs)
PY_INCLUDE  = $(shell python3-config --includes)

PY_LIBFILE  = $(shell python3 -c 'import distutils.sysconfig; print(distutils.sysconfig.get_config_var("LDLIBRARY"))')
CFLAGS += -DPY_LIBFILE='"$(PY_LIBFILE)"'

default: libtclpy$(PACKAGE_VERSION).so

libtclpy$(PACKAGE_VERSION).so: tclpy.o pkgIndex.tcl
	@$(TCLCONFIG_TEST)
	rm -f libtclpy.so tclpy.so
	gcc -shared -fPIC $(CFLAGS) $< -o $@ -Wl,--export-dynamic $(TCL_LIB) $(PY_LIB)
	ln -s $@ libtclpy.so
	ln -s libtclpy.so tclpy.so

tclpy.o: generic/tclpy.c
	@$(TCLCONFIG_TEST)
	gcc -fPIC $(CFLAGS) $(DFLAGS) $(PY_INCLUDE) $(TCL_INCLUDE) -c $< -o $@

pkgIndex.tcl: pkgIndex.tcl.in
	sed "s/PACKAGE_VERSION/$(PACKAGE_VERSION)/g" $< > $@

clean:
	rm -f *.so *.o pkgIndex.tcl

test: default
	TCLLIBPATH=. tclsh tests/tclpy.test

.PHONY: clean test default
