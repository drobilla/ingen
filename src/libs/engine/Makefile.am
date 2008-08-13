SUBDIRS = events

MAINTAINERCLEANFILES = Makefile.in

moduledir = $(libdir)/ingen

module_LTLIBRARIES = libingen_engine.la
libingen_engine_la_CXXFLAGS = \
	@GLIBMM_CFLAGS@ \
	@INGEN_CFLAGS@ \
	@JACK_CFLAGS@ \
	@LASH_CFLAGS@ \
	@LIBLO_CFLAGS@ \
	@RAUL_CFLAGS@ \
	@REDLANDMM_CFLAGS@ \
	@SLV2_CFLAGS@ \
	@LV2_OSC_CFLAGS@

libingen_engine_la_LDFLAGS = -no-undefined -module -avoid-version
libingen_engine_la_LIBADD = \
	../shared/libingen_shared.la \
	@GLIBMM_LIBS@ \
	@JACK_LIBS@ \
	@LASH_LIBS@ \
	@LIBLO_LIBS@ \
	@RAUL_LIBS@ \
	@REDLANDMM_LIBS@ \
	@SLV2_LIBS@ \
	@LV2_OSC_LIBS@

AM_CFLAGS=-std=c99

libingen_engine_la_SOURCES = \
	AudioBuffer.cpp \
	AudioBuffer.hpp \
	AudioDriver.hpp \
	Buffer.cpp \
	Buffer.hpp \
	ClientBroadcaster.cpp \
	ClientBroadcaster.hpp \
	CompiledPatch.hpp \
	ConnectionImpl.cpp \
	ConnectionImpl.hpp \
	Driver.hpp \
	DuplexPort.cpp \
	DuplexPort.hpp \
	Engine.cpp \
	Engine.hpp \
	Event.cpp \
	Event.hpp \
	EventBuffer.cpp \
	EventBuffer.hpp \
	EventSink.cpp \
	EventSink.hpp \
	EventSource.hpp \
	GraphObjectImpl.cpp \
	GraphObjectImpl.hpp \
	InputPort.cpp \
	InputPort.hpp \
	InternalPlugin.cpp \
	InternalPlugin.hpp \
	JackAudioDriver.cpp \
	JackAudioDriver.hpp \
	JackMidiDriver.cpp \
	JackMidiDriver.hpp \
	LADSPAPlugin.cpp \
	LADSPAPlugin.hpp \
	LV2Info.cpp \
	LV2Info.hpp \
	LV2Plugin.cpp \
	LV2Plugin.hpp \
	MidiControlNode.cpp \
	MidiControlNode.hpp \
	MidiDriver.hpp \
	MidiNoteNode.cpp \
	MidiNoteNode.hpp \
	MidiTriggerNode.cpp \
	MidiTriggerNode.hpp \
	NodeBase.cpp \
	NodeBase.hpp \
	NodeFactory.cpp \
	NodeFactory.hpp \
	NodeImpl.hpp \
	OSCClientSender.cpp \
	OSCClientSender.hpp \
	OSCDriver.hpp \
	OSCEngineReceiver.cpp \
	OSCEngineReceiver.hpp \
	ObjectSender.cpp \
	ObjectSender.hpp \
	ObjectStore.cpp \
	ObjectStore.hpp \
	OutputPort.cpp \
	OutputPort.hpp \
	PatchImpl.cpp \
	PatchImpl.hpp \
	PatchPlugin.hpp \
	PluginImpl.cpp \
	PluginImpl.hpp \
	PortImpl.cpp \
	PortImpl.hpp \
	PostProcessor.cpp \
	PostProcessor.hpp \
	ProcessContext.cpp \
	ProcessContext.hpp \
	ProcessSlave.cpp \
	ProcessSlave.hpp \
	QueuedEngineInterface.cpp \
	QueuedEngineInterface.hpp \
	QueuedEvent.cpp \
	QueuedEvent.hpp \
	QueuedEventSource.cpp \
	QueuedEventSource.hpp \
	Responder.hpp \
	ThreadManager.hpp \
	TransportNode.cpp \
	TransportNode.hpp \
	engine.cpp \
	engine.hpp \
	events.hpp \
	events/AllNotesOffEvent.cpp \
	events/AllNotesOffEvent.hpp \
	events/ClearPatchEvent.cpp \
	events/ClearPatchEvent.hpp \
	events/ConnectionEvent.cpp \
	events/ConnectionEvent.hpp \
	events/CreateNodeEvent.cpp \
	events/CreateNodeEvent.hpp \
	events/CreatePatchEvent.cpp \
	events/CreatePatchEvent.hpp \
	events/CreatePortEvent.cpp \
	events/CreatePortEvent.hpp \
	events/DeactivateEvent.cpp \
	events/DeactivateEvent.hpp \
	events/DestroyEvent.cpp \
	events/DestroyEvent.hpp \
	events/DisconnectAllEvent.cpp \
	events/DisconnectAllEvent.hpp \
	events/DisconnectionEvent.cpp \
	events/DisconnectionEvent.hpp \
	events/EnablePatchEvent.cpp \
	events/EnablePatchEvent.hpp \
	events/EnablePortBroadcastingEvent.cpp \
	events/EnablePortBroadcastingEvent.hpp \
	events/LoadPluginsEvent.cpp \
	events/LoadPluginsEvent.hpp \
	events/MidiLearnEvent.cpp \
	events/MidiLearnEvent.hpp \
	events/NoteEvent.cpp \
	events/NoteEvent.hpp \
	events/PingQueuedEvent.hpp \
	events/RegisterClientEvent.cpp \
	events/RegisterClientEvent.hpp \
	events/RenameEvent.cpp \
	events/RenameEvent.hpp \
	events/RequestAllObjectsEvent.cpp \
	events/RequestAllObjectsEvent.hpp \
	events/RequestMetadataEvent.cpp \
	events/RequestMetadataEvent.hpp \
	events/RequestObjectEvent.cpp \
	events/RequestObjectEvent.hpp \
	events/RequestPluginEvent.cpp \
	events/RequestPluginEvent.hpp \
	events/RequestPluginsEvent.cpp \
	events/RequestPluginsEvent.hpp \
	events/RequestPortValueEvent.cpp \
	events/RequestPortValueEvent.hpp \
	events/SendPortActivityEvent.cpp \
	events/SendPortActivityEvent.hpp \
	events/SendPortValueEvent.cpp \
	events/SendPortValueEvent.hpp \
	events/SetMetadataEvent.cpp \
	events/SetMetadataEvent.hpp \
	events/SetPolyphonicEvent.cpp \
	events/SetPolyphonicEvent.hpp \
	events/SetPolyphonyEvent.cpp \
	events/SetPolyphonyEvent.hpp \
	events/SetPortValueEvent.cpp \
	events/SetPortValueEvent.hpp \
	events/UnregisterClientEvent.cpp \
	events/UnregisterClientEvent.hpp \
	jack_compat.h \
	tuning.hpp \
	types.hpp \
	util.hpp

if WITH_LADSPA
libingen_engine_la_SOURCES += \
	LADSPANode.hpp \
	LADSPANode.cpp
endif

if WITH_LV2
libingen_engine_la_SOURCES += \
	LV2Node.hpp \
	LV2Node.cpp
endif

#if WITH_LASH
#libingen_engine_la_SOURCES += \
#	LashDriver.hpp \
#	LashDriver.cpp
#endif

