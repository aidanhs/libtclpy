PACKAGE_VERSION = 0.1

DFLAGS = -DPACKAGE_VERSION='"$(PACKAGE_VERSION)"'

# Flags to improve security
CFLAGS_SEC = \
	-fstack-protector \
	--param=ssp-buffer-size=4 \
	-Wformat \
	-Werror=format-security \
	-D_FORTIFY_SOURCE=2
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

TCLCONFIG=/usr/lib/tclConfig.sh
TCL_LIB     = $(shell . $(TCLCONFIG); echo $$TCL_LIB_SPEC)
TCL_INCLUDE = $(shell . $(TCLCONFIG); echo $$TCL_INCLUDE_SPEC)


default: libtclpy$(PACKAGE_VERSION).so


libtclpy$(PACKAGE_VERSION).so: tclpy.o pkgIndex.tcl
	gcc -shared -fPIC $(CFLAGS) $< -o $@ -Wl,--export-dynamic $(TCL_LIB) \
		-lpthread -ldl -lutil -lm -lpython2.7 -L/usr/lib/python2.7/config

tclpy.o:
	gcc -fPIC $(CFLAGS) $(DFLAGS) \
		-I/usr/include/python2.7 $(TCL_INCLUDE) -c ./generic/tclpy.c -o tclpy.o

pkgIndex.tcl: pkgIndex.tcl.in
	sed "s/PACKAGE_VERSION/$(PACKAGE_VERSION)/g" pkgIndex.tcl.in > pkgIndex.tcl

clean:
	rm -f *.so *.o pkgIndex.tcl

test: default
	TCLLIBPATH=. tclsh tests/tclpy.test

.PHONY: clean test default
