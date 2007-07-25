noinst_LTLIBRARIES = libingen_module.la

libingen_module_la_CXXFLAGS = -I$(top_srcdir)/raul -DINGEN_MODULE_DIR=\"$(libdir)/ingen\" @GLIBMM_CFLAGS@

libingen_module_la_LIBADD = @RAUL_LIBS@ @GLIBMM_LIBS@

libingen_module_la_SOURCES = \
	module.h \
	module.cpp \
	Module.hpp \
	Module.cpp
