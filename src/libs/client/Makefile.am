if BUILD_CLIENT_LIB


moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_client.la

libingen_client_la_CXXFLAGS = \
	-I$(top_srcdir)/slv2 \
	-I$(top_srcdir)/raul \
	-I$(top_srcdir)/ingen/src/common \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	@LIBLO_CFLAGS@ \
	@LXML2_CFLAGS@ @RASQAL_CFLAGS@ @RAPTOR_CFLAGS@ \
	@LSIGCPP_CFLAGS@ @GLIBMM_CFLAGS@

libingen_client_la_LIBADD = \
	@RAUL_LIBS@ @SLV2_LIBS@ @LIBLO_LIBS@ \
	@LXML2_LIBS@ @RASQAL_LIBS@ @RAPTOR_LIBS@ \
	@LSIGCPP_LIBS@ @GLIBMM_LIBS@

libingen_client_la_SOURCES = \
	client.h \
	client.cpp \
	OSCEngineSender.h \
	OSCEngineSender.cpp \
	OSCClientReceiver.h \
	OSCClientReceiver.cpp \
	SigClientInterface.h \
	DirectSigClientInterface.h \
	ThreadedSigClientInterface.h \
	ThreadedSigClientInterface.cpp \
	PresetModel.h \
	ControlModel.h \
	ObjectModel.h \
	ObjectModel.cpp \
	NodeModel.h \
	NodeModel.cpp \
	PortModel.h \
	PortModel.cpp \
	PatchModel.h \
	PatchModel.cpp \
	PluginModel.h \
	PluginModel.cpp \
	Serializer.h \
	Serializer.cpp \
	DeprecatedLoader.h \
	DeprecatedLoader.cpp \
	ConnectionModel.h \
	ConnectionModel.cpp \
	Store.h \
	Store.cpp \
	$(top_srcdir)/ingen/src/common/interface/ClientInterface.h \
	$(top_srcdir)/ingen/src/common/interface/EngineInterface.h

endif # BUILD_CLIENT_LIB

