AM_CXXFLAGS = -I$(top_srcdir)/slv2 -I$(top_srcdir)/raul -I$(top_srcdir)/ingen/src/common -I$(top_srcdir)/ingen/src/libs -DINGEN_MODULE_DIR=\"$(libdir)/ingen\" @GTHREAD_CFLAGS@ @GLIBMM_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = ingen.desktop

bin_PROGRAMS = ingen
ingen_LDADD = @SLV2_LIBS@ @RAUL_LIBS@ @GTHREAD_LIBS@ @GLIBMM_LIBS@ ../../libs/module/libingen_module.la
ingen_DEPENDENCIES = ../../libs/module/libingen_module.la

ingen_SOURCES = \
	main.cpp \
	cmdline.h \
	cmdline.c
