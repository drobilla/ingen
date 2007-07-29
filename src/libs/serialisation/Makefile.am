MAINTAINERCLEANFILES = Makefile.in

moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_serialisation.la

libingen_serialisation_la_CXXFLAGS = -I$(top_srcdir)/ingen/src/common @RAUL_CFLAGS@ @GLIBMM_CFLAGS@
libingen_serialisation_la_LDFLAGS = -no-undefined -module -avoid-version
libingen_serialisation_la_LIBADD = @RAUL_LIBS@ @GLIBMM_LIBS@

libingen_serialisation_la_SOURCES = \
	serialisation.hpp \
	serialisation.cpp \
	Loader.hpp \
	Loader.cpp

