if BUILD_CLIENT_LIB


moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_client.la

libingen_client_la_CXXFLAGS = \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	@INGEN_CFLAGS@ \
	@GLIBMM_CFLAGS@ \
	@LIBLO_CFLAGS@ \
	@LSIGCPP_CFLAGS@ \
	@GLIBMM_CFLAGS@ \
	@LXML2_CFLAGS@ \
	@RAPTOR_CFLAGS@ \
	@RASQAL_CFLAGS@ \
	@RAUL_CFLAGS@ \
	@REDLAND_CFLAGS@ \
	@REDLANDMM_CFLAGS@ \
	@SLV2_CFLAGS@

libingen_client_la_LIBADD = \
	@GLIBMM_LIBS@ \
	@LIBLO_LIBS@ \
	@LSIGCPP_LIBS@ \
	@LXML2_LIBS@ \
	@RAPTOR_LIBS@ \
	@RASQAL_LIBS@ \
	@RAUL_LIBS@ \
	@REDLAND_LIBS@ \
	@REDLANDMM_LIBS@ \
	@SLV2_LIBS@

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
	PluginUI.hpp \
	PluginUI.cpp \
	PortModel.cpp \
	PortModel.hpp \
	PresetModel.hpp \
	SigClientInterface.hpp \
	Store.cpp \
	Store.hpp \
	ThreadedSigClientInterface.cpp \
	ThreadedSigClientInterface.hpp \
	client.cpp \
	client.hpp

endif # BUILD_CLIENT_LIB

