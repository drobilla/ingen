EXTRA_DIST = README

om_demolition_CXXFLAGS = -I$(top_srcdir)/src/clients -I$(top_srcdir)/src/common -DPKGDATADIR=\"$(pkgdatadir)\" $(LXML2_CFLAGS) $(LOSC_CFLAGS)
om_demolition_LDADD = ../libomclient.a $(LOSC_LIBS) $(LXML2_LIBS)

bin_PROGRAMS = om_demolition

om_demolition_DEPENDENCIES = ../libomclient.a

om_demolition_SOURCES = \
	DemolitionClientInterface.h \
	DemolitionClientInterface.cpp \
	DemolitionModel.h \
	DemolitionModel.cpp \
	demolition.cpp \
	cmdline.h \
	cmdline.c
