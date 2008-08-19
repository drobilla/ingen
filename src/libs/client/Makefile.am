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
	@RAUL_CFLAGS@ \
	@REDLANDMM_CFLAGS@ \
	@SLV2_CFLAGS@ \
	@SOUP_CFLAGS@

libingen_client_la_LIBADD = \
	../shared/libingen_shared.la \
	@GLIBMM_LIBS@ \
	@LIBLO_LIBS@ \
	@LSIGCPP_LIBS@ \
	@LXML2_LIBS@ \
	@RAUL_LIBS@ \
	@REDLANDMM_LIBS@ \
	@SLV2_LIBS@ \
	@SOUP_LIBS@

libingen_client_la_SOURCES = \
	$(top_srcdir)/ingen/src/common/interface/ClientInterface.hpp \
	$(top_srcdir)/ingen/src/common/interface/EngineInterface.hpp \
	ClientStore.cpp \
	ClientStore.hpp \
	ConnectionModel.hpp \
	DeprecatedLoader.cpp \
	DeprecatedLoader.hpp \
	HTTPClientReceiver.cpp \
	HTTPClientReceiver.hpp \
	HTTPEngineSender.cpp \
	HTTPEngineSender.hpp \
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
	PluginUI.cpp \
	PluginUI.hpp \
	PortModel.cpp \
	PortModel.hpp \
	SigClientInterface.hpp \
	ThreadedSigClientInterface.cpp \
	ThreadedSigClientInterface.hpp \
	client.cpp \
	client.hpp

endif # BUILD_CLIENT_LIB

