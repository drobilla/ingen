EXTRA_DIST = README

om_patch_loader_CXXFLAGS = -I$(top_srcdir)/src/clients -I$(top_srcdir)/src/common/util -I$(top_srcdir)/src/common/interface -DPKGDATADIR=\"$(pkgdatadir)\" $(LXML2_CFLAGS) $(LOSC_CFLAGS)
om_patch_loader_LDADD = ../libomclient.a $(LOSC_LIBS) $(LXML2_LIBS)

bin_PROGRAMS = om_patch_loader

om_patch_loader_DEPENDENCIES = ../libomclient.a

om_patch_loader_SOURCES = \
	patch_loader.cpp \
	cmdline.h \
	cmdline.c
