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
	$(top_srcdir)/ingen/src/common/interface/ClientInterface.hpp \
	$(top_srcdir)/ingen/src/common/interface/EngineInterface.hpp \
	ConnectionModel.hpp \
	ControlModel.hpp \
	DeprecatedLoader.cpp \
	DeprecatedLoader.hpp \
	DirectSigClientInterface.hpp \
	NodeModel.cpp \
	NodeModel.hpp \
	OSCClientReceiver.cpp \
	OSCClientReceiver.hpp \
	OSCEngineSender.cpp \
	OSCEngineSender.hpp \
	ObjectModel.cpp \
	ObjectModel.hpp \
	PatchModel.cpp \
	PatchModel.hpp \
	PluginModel.cpp \
	PluginModel.hpp \
	PortModel.cpp \
	PortModel.hpp \
	PresetModel.hpp \
	Serializer.cpp \
	Serializer.hpp \
	SigClientInterface.hpp \
	Store.cpp \
	Store.hpp \
	ThreadedSigClientInterface.cpp \
	ThreadedSigClientInterface.hpp \
	client.cpp \
	client.hpp

endif # BUILD_CLIENT_LIB

