noinst_LTLIBRARIES = libingen_module.la

libingen_module_la_CXXFLAGS = \
	@INGEN_CFLAGS@ \
	@GLIBMM_CFLAGS@ \
	@REDLANDMM_CFLAGS@ \
	@RAUL_CFLAGS@ \
	@SLV2_CFLAGS@ \
	-DINGEN_MODULE_DIR=\"$(libdir)/ingen\"

libingen_module_la_LIBADD = \
	../shared/libingen_shared.la \
	@GLIBMM_LIBS@ \
	@REDLANDMM_LIBS@ \
	@RAUL_LIBS@ \
	@SLV2_LIBS@

libingen_module_la_SOURCES = \
	global.hpp \
	global.cpp \
	World.hpp \
	Module.hpp \
	Module.cpp
