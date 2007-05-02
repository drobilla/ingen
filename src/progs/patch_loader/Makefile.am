EXTRA_DIST = README

ingen_load_CXXFLAGS = -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/common @LSIGCPP_CFLAGS@ @RAUL_CFLAGS@ @GLIBMM_CFLAGS@ @RASQAL_CFLAGS@ -DINGEN_MODULE_DIR=\"$(libdir)/ingen\"
ingen_load_LDADD = ../../libs/client/libingen_client.la ../../libs/module/libingen_module.la @RAUL_LIBS@ @LSIGCPP_LIBS@ @GLIBMM_LIBS@ @RASQAL_LIBS@

bin_PROGRAMS = ingen_load

#ingen_load_DEPENDENCIES = ../../libs/client/libingen_client.la

ingen_load_SOURCES = \
	patch_loader.cpp \
	cmdline.h \
	cmdline.c
