noinst_LTLIBRARIES = libingen_module.la

libingen_module_la_CXXFLAGS = -I$(top_srcdir)/raul -I$(top_srcdir)/slv2 -DINGEN_MODULE_DIR=\"$(libdir)/ingen\" @GLIBMM_CFLAGS@

libingen_module_la_LIBADD = @RAUL_LIBS@ @GLIBMM_LIBS@ @SLV2_LIBS@

libingen_module_la_SOURCES = \
	module.h \
	module.cpp \
	World.hpp \
	Module.hpp \
	Module.cpp
