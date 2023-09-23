/*
  This file is part of Ingen.
  Copyright 2007-2017 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/URIs.hpp"

#include "ingen/Forge.hpp"
#include "ingen/ingen.h"
#include "lilv/lilv.h"
#include "lv2/atom/atom.h"
#include "lv2/buf-size/buf-size.h"
#include "lv2/core/lv2.h"
#include "lv2/log/log.h"
#include "lv2/midi/midi.h"
#include "lv2/morph/morph.h"
#include "lv2/options/options.h"
#include "lv2/parameters/parameters.h"
#include "lv2/patch/patch.h"
#include "lv2/port-props/port-props.h"
#include "lv2/presets/presets.h"
#include "lv2/resize-port/resize-port.h"
#include "lv2/state/state.h"
#include "lv2/time/time.h"
#include "lv2/worker/worker.h"

namespace ingen {

URIs::Quark::Quark(Forge&      ingen_forge,
                   URIMap*,
                   LilvWorld*  lworld,
                   const char* str)
	: URI(str)
	, _urid_atom(ingen_forge.make_urid(URI(str)))
	, _uri_atom(ingen_forge.alloc_uri(str))
	, _lilv_node(lilv_new_uri(lworld, str))
{}

URIs::Quark::Quark(const Quark& copy)
	: URI(copy)
	, _urid_atom(copy._urid_atom)
	, _uri_atom(copy._uri_atom)
	, _lilv_node(lilv_node_duplicate(copy._lilv_node))
{}

URIs::Quark::~Quark()
{
	lilv_node_free(_lilv_node);
}

#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

URIs::URIs(Forge& ingen_forge, URIMap* map, LilvWorld* lworld)
	: forge(ingen_forge)
	, atom_AtomPort         (forge, map, lworld, LV2_ATOM__AtomPort)
	, atom_Bool             (forge, map, lworld, LV2_ATOM__Bool)
	, atom_Chunk            (forge, map, lworld, LV2_ATOM__Chunk)
	, atom_Float            (forge, map, lworld, LV2_ATOM__Float)
	, atom_Int              (forge, map, lworld, LV2_ATOM__Int)
	, atom_Object           (forge, map, lworld, LV2_ATOM__Object)
	, atom_Path             (forge, map, lworld, LV2_ATOM__Path)
	, atom_Sequence         (forge, map, lworld, LV2_ATOM__Sequence)
	, atom_Sound            (forge, map, lworld, LV2_ATOM__Sound)
	, atom_String           (forge, map, lworld, LV2_ATOM__String)
	, atom_URI              (forge, map, lworld, LV2_ATOM__URI)
	, atom_URID             (forge, map, lworld, LV2_ATOM__URID)
	, atom_bufferType       (forge, map, lworld, LV2_ATOM__bufferType)
	, atom_eventTransfer    (forge, map, lworld, LV2_ATOM__eventTransfer)
	, atom_supports         (forge, map, lworld, LV2_ATOM__supports)
	, bufsz_maxBlockLength  (forge, map, lworld, LV2_BUF_SIZE__maxBlockLength)
	, bufsz_minBlockLength  (forge, map, lworld, LV2_BUF_SIZE__minBlockLength)
	, bufsz_sequenceSize    (forge, map, lworld, LV2_BUF_SIZE__sequenceSize)
	, doap_name             (forge, map, lworld, "http://usefulinc.com/ns/doap#name")
	, ingen_Arc             (forge, map, lworld, INGEN__Arc)
	, ingen_Block           (forge, map, lworld, INGEN__Block)
	, ingen_BundleEnd       (forge, map, lworld, INGEN__BundleEnd)
	, ingen_BundleStart     (forge, map, lworld, INGEN__BundleStart)
	, ingen_Graph           (forge, map, lworld, INGEN__Graph)
	, ingen_GraphPrototype  (forge, map, lworld, INGEN__GraphPrototype)
	, ingen_Internal        (forge, map, lworld, INGEN__Internal)
	, ingen_Redo            (forge, map, lworld, INGEN__Redo)
	, ingen_Undo            (forge, map, lworld, INGEN__Undo)
	, ingen_activity        (forge, map, lworld, INGEN__activity)
	, ingen_arc             (forge, map, lworld, INGEN__arc)
	, ingen_block           (forge, map, lworld, INGEN__block)
	, ingen_broadcast       (forge, map, lworld, INGEN__broadcast)
	, ingen_canvasX         (forge, map, lworld, INGEN__canvasX)
	, ingen_canvasY         (forge, map, lworld, INGEN__canvasY)
	, ingen_enabled         (forge, map, lworld, INGEN__enabled)
	, ingen_externalContext (forge, map, lworld, INGEN__externalContext)
	, ingen_file            (forge, map, lworld, INGEN__file)
	, ingen_head            (forge, map, lworld, INGEN__head)
	, ingen_incidentTo      (forge, map, lworld, INGEN__incidentTo)
	, ingen_internalContext (forge, map, lworld, INGEN__internalContext)
	, ingen_loadedBundle    (forge, map, lworld, INGEN__loadedBundle)
	, ingen_maxRunLoad      (forge, map, lworld, INGEN__maxRunLoad)
	, ingen_meanRunLoad     (forge, map, lworld, INGEN__meanRunLoad)
	, ingen_minRunLoad      (forge, map, lworld, INGEN__minRunLoad)
	, ingen_numThreads      (forge, map, lworld, INGEN__numThreads)
	, ingen_polyphonic      (forge, map, lworld, INGEN__polyphonic)
	, ingen_polyphony       (forge, map, lworld, INGEN__polyphony)
	, ingen_prototype       (forge, map, lworld, INGEN__prototype)
	, ingen_sprungLayout    (forge, map, lworld, INGEN__sprungLayout)
	, ingen_tail            (forge, map, lworld, INGEN__tail)
	, ingen_uiEmbedded      (forge, map, lworld, INGEN__uiEmbedded)
	, ingen_value           (forge, map, lworld, INGEN__value)
	, log_Error             (forge, map, lworld, LV2_LOG__Error)
	, log_Note              (forge, map, lworld, LV2_LOG__Note)
	, log_Trace             (forge, map, lworld, LV2_LOG__Trace)
	, log_Warning           (forge, map, lworld, LV2_LOG__Warning)
	, lv2_AudioPort         (forge, map, lworld, LV2_CORE__AudioPort)
	, lv2_CVPort            (forge, map, lworld, LV2_CORE__CVPort)
	, lv2_ControlPort       (forge, map, lworld, LV2_CORE__ControlPort)
	, lv2_InputPort         (forge, map, lworld, LV2_CORE__InputPort)
	, lv2_OutputPort        (forge, map, lworld, LV2_CORE__OutputPort)
	, lv2_Plugin            (forge, map, lworld, LV2_CORE__Plugin)
	, lv2_appliesTo         (forge, map, lworld, LV2_CORE__appliesTo)
	, lv2_binary            (forge, map, lworld, LV2_CORE__binary)
	, lv2_connectionOptional(forge, map, lworld, LV2_CORE__connectionOptional)
	, lv2_control           (forge, map, lworld, LV2_CORE__control)
	, lv2_default           (forge, map, lworld, LV2_CORE__default)
	, lv2_designation       (forge, map, lworld, LV2_CORE__designation)
	, lv2_enumeration       (forge, map, lworld, LV2_CORE__enumeration)
	, lv2_extensionData     (forge, map, lworld, LV2_CORE__extensionData)
	, lv2_index             (forge, map, lworld, LV2_CORE__index)
	, lv2_integer           (forge, map, lworld, LV2_CORE__integer)
	, lv2_maximum           (forge, map, lworld, LV2_CORE__maximum)
	, lv2_microVersion      (forge, map, lworld, LV2_CORE__microVersion)
	, lv2_minimum           (forge, map, lworld, LV2_CORE__minimum)
	, lv2_minorVersion      (forge, map, lworld, LV2_CORE__minorVersion)
	, lv2_name              (forge, map, lworld, LV2_CORE__name)
	, lv2_port              (forge, map, lworld, LV2_CORE__port)
	, lv2_portProperty      (forge, map, lworld, LV2_CORE__portProperty)
	, lv2_prototype         (forge, map, lworld, LV2_CORE__prototype)
	, lv2_sampleRate        (forge, map, lworld, LV2_CORE__sampleRate)
	, lv2_scalePoint        (forge, map, lworld, LV2_CORE__scalePoint)
	, lv2_symbol            (forge, map, lworld, LV2_CORE__symbol)
	, lv2_toggled           (forge, map, lworld, LV2_CORE__toggled)
	, midi_Bender           (forge, map, lworld, LV2_MIDI__Bender)
	, midi_ChannelPressure  (forge, map, lworld, LV2_MIDI__ChannelPressure)
	, midi_Controller       (forge, map, lworld, LV2_MIDI__Controller)
	, midi_MidiEvent        (forge, map, lworld, LV2_MIDI__MidiEvent)
	, midi_NoteOn           (forge, map, lworld, LV2_MIDI__NoteOn)
	, midi_binding          (forge, map, lworld, LV2_MIDI__binding)
	, midi_controllerNumber (forge, map, lworld, LV2_MIDI__controllerNumber)
	, midi_noteNumber       (forge, map, lworld, LV2_MIDI__noteNumber)
	, midi_channel          (forge, map, lworld, LV2_MIDI__channel)
	, morph_AutoMorphPort   (forge, map, lworld, LV2_MORPH__AutoMorphPort)
	, morph_MorphPort       (forge, map, lworld, LV2_MORPH__MorphPort)
	, morph_currentType     (forge, map, lworld, LV2_MORPH__currentType)
	, morph_supportsType    (forge, map, lworld, LV2_MORPH__supportsType)
	, opt_interface         (forge, map, lworld, LV2_OPTIONS__interface)
	, param_sampleRate      (forge, map, lworld, LV2_PARAMETERS__sampleRate)
	, patch_Copy            (forge, map, lworld, LV2_PATCH__Copy)
	, patch_Delete          (forge, map, lworld, LV2_PATCH__Delete)
	, patch_Get             (forge, map, lworld, LV2_PATCH__Get)
	, patch_Message         (forge, map, lworld, LV2_PATCH__Message)
	, patch_Move            (forge, map, lworld, LV2_PATCH__Move)
	, patch_Patch           (forge, map, lworld, LV2_PATCH__Patch)
	, patch_Put             (forge, map, lworld, LV2_PATCH__Put)
	, patch_Response        (forge, map, lworld, LV2_PATCH__Response)
	, patch_Set             (forge, map, lworld, LV2_PATCH__Set)
	, patch_add             (forge, map, lworld, LV2_PATCH__add)
	, patch_body            (forge, map, lworld, LV2_PATCH__body)
	, patch_context         (forge, map, lworld, LV2_PATCH__context)
	, patch_destination     (forge, map, lworld, LV2_PATCH__destination)
	, patch_property        (forge, map, lworld, LV2_PATCH__property)
	, patch_remove          (forge, map, lworld, LV2_PATCH__remove)
	, patch_sequenceNumber  (forge, map, lworld, LV2_PATCH__sequenceNumber)
	, patch_subject         (forge, map, lworld, LV2_PATCH__subject)
	, patch_value           (forge, map, lworld, LV2_PATCH__value)
	, patch_wildcard        (forge, map, lworld, LV2_PATCH__wildcard)
	, pprops_logarithmic    (forge, map, lworld, LV2_PORT_PROPS__logarithmic)
	, pset_Preset           (forge, map, lworld, LV2_PRESETS__Preset)
	, pset_preset           (forge, map, lworld, LV2_PRESETS__preset)
	, rdf_type              (forge, map, lworld, NS_RDF "type")
	, rdfs_Class            (forge, map, lworld, NS_RDFS "Class")
	, rdfs_label            (forge, map, lworld, NS_RDFS "label")
	, rdfs_seeAlso          (forge, map, lworld, NS_RDFS "seeAlso")
	, rsz_minimumSize       (forge, map, lworld, LV2_RESIZE_PORT__minimumSize)
	, state_loadDefaultState(forge, map, lworld, LV2_STATE__loadDefaultState)
	, state_state           (forge, map, lworld, LV2_STATE__state)
	, time_Position         (forge, map, lworld, LV2_TIME__Position)
	, time_bar              (forge, map, lworld, LV2_TIME__bar)
	, time_barBeat          (forge, map, lworld, LV2_TIME__barBeat)
	, time_beatUnit         (forge, map, lworld, LV2_TIME__beatUnit)
	, time_beatsPerBar      (forge, map, lworld, LV2_TIME__beatsPerBar)
	, time_beatsPerMinute   (forge, map, lworld, LV2_TIME__beatsPerMinute)
	, time_frame            (forge, map, lworld, LV2_TIME__frame)
	, time_speed            (forge, map, lworld, LV2_TIME__speed)
	, work_schedule         (forge, map, lworld, LV2_WORKER__schedule)
{}

} // namespace ingen
