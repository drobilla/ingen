EXTRA_DIST = README

om_patch_loader_CXXFLAGS = -I$(top_srcdir)/src/libs/client -I$(top_srcdir)/src/common -DPKGDATADIR=\"$(pkgdatadir)\" $(LXML2_CFLAGS) $(LOSC_CFLAGS)
om_patch_loader_LDADD = ../../libs/client/libomclient.la $(LOSC_LIBS) $(LXML2_LIBS)

bin_PROGRAMS = om_patch_loader

om_patch_loader_DEPENDENCIES = ../../libs/client/libomclient.la

om_patch_loader_SOURCES = \
	patch_loader.cpp \
	cmdline.h \
	cmdline.c
