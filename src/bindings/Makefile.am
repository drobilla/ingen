FLAGS = @RAUL_CFLAGS@ @GLIBMM_CFLAGS@ -I$(top_srcdir)/ingen/src/common
LIBS = @RAUL_LIBS@ @GLIBMM_LIBS@ -lpython2.4
AM_CXXFLAGS = -I/usr/include/python2.4 $(FLAGS)

EXTRA_DIST = ingen.i test_ingen.py

if WITH_SWIG
bindingsdir = $(libdir)/ingen
bindings_LTLIBRARIES = libingen_bindings.la
libingen_bindings_la_LDFLAGS = -no-undefined -module -avoid-version
libingen_bindings_la_LIBADD = @RAUL_LIBS@ @GLIBMM_LIBS@ #../libs/module/libingen_module.la
libingen_bindings_la_SOURCES = ingen_wrap.cxx ingen_bindings.cpp ingen_bindings.hpp
libingen_bindings_la_CXXFLAGS = \
	@SLV2_CFLAGS@ @RAUL_CFLAGS@ @GLIBMM_CFLAGS@ \
	-I$(top_srcdir)/ingen/src/common \
	-I$(top_srcdir)/ingen/src/libs \
	-I/usr/include/python2.4

swig: ingen.i
	swig -c++ -Wall $(FLAGS) -python ingen.i

ingen_wrap.cxx: swig

libingen_bindings_la: swig ingen_wrap.cxx ingen_bindings.cpp

all-local: swig
	ln -sf .libs/libingen_bindings.so _ingen.so

#libtool --mode=compile g++ -fPIC -shared $(CXXFLAGS) $(FLAGS) $(LIBS) -I/usr/include/python2.4 ingen_wrap.cxx -o libingen_python.so.0.0.0
endif

clean-local:
	rm -f *.cxx
	rm -f *.so
	rm -f *.o
	rm -f ingen.py
