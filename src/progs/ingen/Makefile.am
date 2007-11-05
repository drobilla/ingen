AM_CXXFLAGS = @GTHREAD_CFLAGS@ @GLIBMM_CFLAGS@ @RAUL_CFLAGS@ @SLV2_CFLAGS@ @INGEN_CFLAGS@ -DINGEN_MODULE_DIR=\"$(libdir)/ingen\"

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
