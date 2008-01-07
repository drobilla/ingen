noinst_LTLIBRARIES = libingen_module.la

libingen_module_la_CXXFLAGS = @GLIBMM_CFLAGS@ @REDLAND_CFLAGS@ @REDLANDMM_CFLAGS@ @RAUL_CFLAGS@ @SLV2_CFLAGS@ -DINGEN_MODULE_DIR=\"$(libdir)/ingen\"

libingen_module_la_LIBADD = @GLIBMM_LIBS@ @REDLAND_LIBS@ @REDLANDMM_LIBS@ @RAUL_LIBS@ @SLV2_LIBS@

libingen_module_la_SOURCES = \
	global.hpp \
	global.cpp \
	World.hpp \
	Module.hpp \
	Module.cpp
