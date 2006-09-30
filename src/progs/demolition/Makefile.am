EXTRA_DIST = README

om_demolition_CXXFLAGS = -I$(top_srcdir)/src/libs/client -I$(top_srcdir)/src/common -DPKGDATADIR=\"$(pkgdatadir)\" $(LXML2_CFLAGS) $(LOSC_CFLAGS) $(LSIGCPP_CFLAGS)
om_demolition_LDADD = ../../libs/client/libingenclient.la $(LOSC_LIBS) $(LXML2_LIBS) $(LSIGCPP_LIBS)

bin_PROGRAMS = om_demolition

om_demolition_DEPENDENCIES = ../../libs/client/libingenclient.la

om_demolition_SOURCES = \
	demolition.cpp \
	cmdline.h \
	cmdline.c
