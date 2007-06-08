AM_CXXFLAGS = @RAUL_CFLAGS@ @GTHREAD_CFLAGS@ -I$(top_srcdir)/src/common -I$(top_srcdir)/src/libs -DINGEN_MODULE_DIR=\"$(libdir)/ingen\"

MAINTAINERCLEANFILES = Makefile.in
EXTRA_DIST = ingen_dev

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = ingen.desktop

bin_PROGRAMS = ingen
ingen_LDADD = @RAUL_LIBS@ @GTHREAD_LIBS@../../libs/module/libingen_module.la
ingen_DEPENDENCIES = ../../libs/module/libingen_module.la

ingen_SOURCES = \
	main.cpp \
	cmdline.h \
	cmdline.c
