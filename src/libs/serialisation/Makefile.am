MAINTAINERCLEANFILES = Makefile.in

moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_serialisation.la

libingen_serialisation_la_CXXFLAGS = @RAUL_CFLAGS@ -I$(top_srcdir)/src/common
libingen_serialisation_la_LDFLAGS = -no-undefined -module -avoid-version
libingen_serialisation_la_LIBADD = @RAUL_LIBS@

libingen_serialisation_la_SOURCES = \
	serialisation.h \
	serialisation.cpp \
	Loader.h \
	Loader.cpp

