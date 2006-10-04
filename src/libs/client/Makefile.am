AM_CXXFLAGS = -I$(top_srcdir)/src/common

if BUILD_CLIENT_LIB
noinst_LTLIBRARIES = libingenclient.la

libingenclient_la_CXXFLAGS = -I$(top_srcdir)/src/common -DPKGDATADIR=\"$(pkgdatadir)\" @LXML2_CFLAGS@ @RAPTOR_CFLAGS@ @LSIGCPP_CFLAGS@
libingenclient_la_LIBADD = @LXML2_LIBS@ @LOSC_LIBS@ @RAPTOR_LIBS@ @LSIGCPP_LIBS@

libingenclient_la_SOURCES = \
	OSCEngineSender.h \
	OSCEngineSender.cpp \
	OSCModelEngineInterface.h \
	OSCClientReceiver.h \
	OSCClientReceiver.cpp \
	SigClientInterface.h \
	DirectSigClientInterface.h \
	ThreadedSigClientInterface.h \
	ThreadedSigClientInterface.cpp \
	ModelEngineInterface.h \
	ModelEngineInterface.cpp \
	PresetModel.h \
	ControlModel.h \
	ObjectModel.h \
	ObjectModel.cpp \
	NodeModel.h \
	NodeModel.cpp \
	PortModel.h \
	PatchModel.h \
	PatchModel.cpp \
	PluginModel.h \
	Serializer.h \
	Serializer.cpp \
	DeprecatedSerializer.h \
	DeprecatedSerializer.cpp \
	ConnectionModel.h \
	ConnectionModel.cpp \
	Store.h \
	Store.cpp \
	$(top_srcdir)/src/common/util/Path.h \
	$(top_srcdir)/src/common/interface/ClientInterface.h \
	$(top_srcdir)/src/common/interface/EngineInterface.h

endif # BUILD_CLIENT_LIB

