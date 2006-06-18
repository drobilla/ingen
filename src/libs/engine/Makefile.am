SUBDIRS = tests
DIST_SUBDIRS = events

AM_CXXFLAGS = @JACK_CFLAGS@ @LOSC_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@ @SLV2_CFLAGS@ -I$(top_srcdir)/src/common -I$(top_srcdir)/src/libs/engine/events -fno-exceptions -fno-rtti

MAINTAINERCLEANFILES = Makefile.in

noinst_LTLIBRARIES = libom.la

libom_la_SOURCES = \
	util.h \
	tuning.h \
	DataType.h \
	Node.h \
	NodeBase.h \
	NodeBase.cpp \
	InternalNode.h \
	Patch.h \
	Patch.cpp \
	NodeFactory.h \
	NodeFactory.cpp \
	Om.h \
	Om.cpp \
	OmApp.h \
	OmApp.cpp \
	JackAudioDriver.h \
	JackAudioDriver.cpp \
	OSCReceiver.h \
	OSCReceiver.cpp \
	Responder.h \
	OSCResponder.h \
	OSCResponder.cpp \
	ClientKey.h \
	ClientBroadcaster.h \
	ClientBroadcaster.cpp \
	ObjectSender.h \
	ObjectSender.cpp \
	OSCClient.h \
	OSCClient.cpp \
	Buffer.h \
	Buffer.cpp \
	Port.h \
	Port.cpp \
	PortBase.h \
	PortBase.cpp \
	InputPort.h \
	InputPort.cpp \
	OutputPort.h \
	OutputPort.cpp \
	MidiMessage.h \
	MidiNoteNode.h \
	MidiNoteNode.cpp \
	MidiTriggerNode.h \
	MidiTriggerNode.cpp \
	MidiControlNode.h \
	MidiControlNode.cpp \
	Event.h \
	Event.cpp \
	QueuedEvent.h \
	EventSource.h \
	QueuedEventSource.h \
	QueuedEventSource.cpp \
	QueuedEngineInterface.h \
	QueuedEngineInterface.cpp \
	OmObject.h \
	Maid.h \
	Maid.cpp \
	MaidObject.h \
	Tree.h \
	ClientRecord.h \
	PluginLibrary.h \
	Plugin.h \
	Array.h \
	List.h \
	PostProcessor.h \
	PostProcessor.cpp \
	Connection.h \
	Connection.cpp \
	ConnectionBase.h \
	ConnectionBase.cpp \
	ObjectStore.h \
	ObjectStore.cpp \
	TransportNode.h \
	TransportNode.cpp \
	Driver.h \
	AudioDriver.h \
	MidiDriver.h \
	midi.h \
	../libom_la/util/Semaphore.h \
	../libom_la/util/types.h \
	../libom_la/util/Path.h \
	../libom_la/util/Queue.h \
	../libom_la/interface/ClientInterface.h \
	../libom_la/interface/EngineInterface.h \
	instantiations.cpp \
	events/RegisterClientEvent.h \
	events/RegisterClientEvent.cpp \
	events/UnregisterClientEvent.h \
	events/UnregisterClientEvent.cpp \
	events/PingQueuedEvent.h \
	events/ActivateEvent.h \
	events/ActivateEvent.cpp \
	events/DeactivateEvent.h \
	events/DeactivateEvent.cpp \
	events/SetPortValueEvent.h \
	events/SetPortValueEvent.cpp \
	events/SetPortValueQueuedEvent.h \
	events/SetPortValueQueuedEvent.cpp \
	events/NoteOnEvent.h \
	events/NoteOnEvent.cpp \
	events/NoteOffEvent.h \
	events/NoteOffEvent.cpp \
	events/AllNotesOffEvent.h \
	events/AllNotesOffEvent.cpp \
	events/ConnectionEvent.h \
	events/ConnectionEvent.cpp \
	events/DisconnectionEvent.h \
	events/DisconnectionEvent.cpp \
	events/DisconnectNodeEvent.h \
	events/DisconnectNodeEvent.cpp \
	events/DisconnectPortEvent.h \
	events/DisconnectPortEvent.cpp \
	events/DestroyEvent.h \
	events/DestroyEvent.cpp \
	events/AddPortEvent.h \
	events/AddPortEvent.cpp \
	events/AddNodeEvent.h \
	events/AddNodeEvent.cpp \
	events/SetMetadataEvent.h \
	events/SetMetadataEvent.cpp \
	events/RequestMetadataEvent.h \
	events/RequestMetadataEvent.cpp \
	events/RequestPortValueEvent.h \
	events/RequestPortValueEvent.cpp \
	events/RequestAllObjectsEvent.h \
	events/RequestAllObjectsEvent.cpp \
	events/RequestPluginsEvent.h \
	events/RequestPluginsEvent.cpp \
	events/CreatePatchEvent.h \
	events/CreatePatchEvent.cpp \
	events/LoadPluginsEvent.h \
	events/LoadPluginsEvent.cpp \
	events/EnablePatchEvent.h \
	events/EnablePatchEvent.cpp \
	events/DisablePatchEvent.h \
	events/DisablePatchEvent.cpp \
	events/ClearPatchEvent.h \
	events/ClearPatchEvent.cpp \
	events/RenameEvent.h \
	events/RenameEvent.cpp \
	events/MidiLearnEvent.h \
	events/MidiLearnEvent.cpp \
	types.h
# 	BridgeNode.h 
# 	BridgeNode.cpp 
# 	ControlInputNode.h 
# 	ControlInputNode.cpp 
# 	ControlOutputNode.h 
# 	ControlOutputNode.cpp 
# 	AudioInputNode.h 
# 	AudioInputNode.cpp 
# 	AudioOutputNode.h 
# 	AudioOutputNode.cpp 
# 	MidiInputNode.h 
# 	MidiInputNode.cpp 
# 	MidiOutputNode.h 
# 	MidiOutputNode.cpp 

if WITH_JACK_MIDI
libom_la_SOURCES += \
	JackMidiDriver.h \
	JackMidiDriver.cpp
endif

if WITH_ALSA_MIDI
libom_la_SOURCES += \
	AlsaMidiDriver.h \
	AlsaMidiDriver.cpp
endif

if WITH_LADSPA
libom_la_SOURCES += \
	LADSPAPlugin.h \
	LADSPAPlugin.cpp
endif

if WITH_DSSI
libom_la_SOURCES += \
	DSSIPlugin.h \
	DSSIPlugin.cpp \
	events/DSSIConfigureEvent.cpp \
	events/DSSIConfigureEvent.h \
	events/DSSIControlEvent.cpp \
	events/DSSIControlEvent.h \
	events/DSSIProgramEvent.cpp \
	events/DSSIProgramEvent.h \
	events/DSSIUpdateEvent.cpp \
	events/DSSIUpdateEvent.h
endif

if WITH_LV2
libom_la_SOURCES += \
	LV2Plugin.h \
	LV2Plugin.cpp
endif

if WITH_LASH
libom_la_SOURCES += \
	LashDriver.h \
	LashDriver.cpp \
	LashRestoreDoneEvent.h
endif

