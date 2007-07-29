noinst_LTLIBRARIES = libingen_module.la

libingen_module_la_CXXFLAGS = @RAUL_CFLAGS@ @SLV2_CFLAGS@ @GLIBMM_CFLAGS@ -DINGEN_MODULE_DIR=\"$(libdir)/ingen\"

libingen_module_la_LIBADD = @RAUL_LIBS@ @GLIBMM_LIBS@ @SLV2_LIBS@

libingen_module_la_SOURCES = \
	module.h \
	module.cpp \
	World.hpp \
	Module.hpp \
	Module.cpp
