EXTRA_DIST = README

ingen_load_CXXFLAGS = -I$(top_srcdir)/src/libs/client -I$(top_srcdir)/src/common @LSIGCPP_CFLAGS@ @RAUL_CFLAGS@ @GLIBMM_CFLAGS@ @RASQAL_CFLAGS@
ingen_load_LDADD = ../../libs/client/libingenclient.la  @RAUL_LIBS@ @LSIGCPP_LIBS@ @GLIBMM_LIBS@ @RASQAL_LIBS@

bin_PROGRAMS = ingen_load

#ingen_load_DEPENDENCIES = ../../libs/client/libingenclient.la

ingen_load_SOURCES = \
	patch_loader.cpp \
	cmdline.h \
	cmdline.c
