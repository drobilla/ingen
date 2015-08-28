/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/URIMap.hpp"
#include "ingen/URIs.hpp"
#include "ingen/ingen.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

namespace Ingen {

URIs::Quark::Quark(Forge&      forge,
                   URIMap*     map,
                   LilvWorld*  lworld,
                   const char* c_str)
	: Raul::URI(c_str)
	, urid(forge.make_urid(Raul::URI(c_str)))
	, uri(forge.alloc_uri(c_str))
	, lnode(lilv_new_uri(lworld, c_str))
{}

URIs::Quark::Quark(const Quark& copy)
	: Raul::URI(copy)
	, urid(copy.urid)
	, uri(copy.uri)
	, lnode(lilv_node_duplicate(copy.lnode))
{}

URIs::Quark::~Quark()
{
	lilv_node_free(lnode);
}

#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

URIs::URIs(Forge& f, URIMap* map, LilvWorld* lworld)
	: forge(f)
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
	, atom_Vector           (forge, map, lworld, LV2_ATOM__Vector)
	, atom_bufferType       (forge, map, lworld, LV2_ATOM__bufferType)
	, atom_eventTransfer    (forge, map, lworld, LV2_ATOM__eventTransfer)
	, atom_supports         (forge, map, lworld, LV2_ATOM__supports)
	, bufsz_maxBlockLength  (forge, map, lworld, LV2_BUF_SIZE__maxBlockLength)
	, bufsz_minBlockLength  (forge, map, lworld, LV2_BUF_SIZE__minBlockLength)
	, bufsz_sequenceSize    (forge, map, lworld, LV2_BUF_SIZE__sequenceSize)
	, doap_name             (forge, map, lworld, "http://usefulinc.com/ns/doap#name")
	, ingen_Arc             (forge, map, lworld, INGEN__Arc)
	, ingen_Block           (forge, map, lworld, INGEN__Block)
	, ingen_Graph           (forge, map, lworld, INGEN__Graph)
	, ingen_GraphPrototype  (forge, map, lworld, INGEN__GraphPrototype)
	, ingen_Internal        (forge, map, lworld, INGEN__Internal)
	, ingen_activity        (forge, map, lworld, INGEN__activity)
	, ingen_arc             (forge, map, lworld, INGEN__arc)
	, ingen_block           (forge, map, lworld, INGEN__block)
	, ingen_broadcast       (forge, map, lworld, INGEN__broadcast)
	, ingen_canvasX         (forge, map, lworld, INGEN__canvasX)
	, ingen_canvasY         (forge, map, lworld, INGEN__canvasY)
	, ingen_enabled         (forge, map, lworld, INGEN__enabled)
	, ingen_file            (forge, map, lworld, INGEN__file)
	, ingen_head            (forge, map, lworld, INGEN__head)
	, ingen_incidentTo      (forge, map, lworld, INGEN__incidentTo)
	, ingen_polyphonic      (forge, map, lworld, INGEN__polyphonic)
	, ingen_polyphony       (forge, map, lworld, INGEN__polyphony)
	, ingen_prototype       (forge, map, lworld, INGEN__prototype)
	, ingen_sprungLayout    (forge, map, lworld, INGEN__sprungLayout)
	, ingen_tail            (forge, map, lworld, INGEN__tail)
	, ingen_uiEmbedded      (forge, map, lworld, INGEN__uiEmbedded)
	, ingen_value           (forge, map, lworld, INGEN__value)
	, log_Error             (forge, map, lworld, LV2_LOG__Error)
	, log_Note              (forge, map, lworld, LV2_LOG__Note)
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
	, lv2_default           (forge, map, lworld, LV2_CORE__default)
	, lv2_designation       (forge, map, lworld, LV2_CORE__designation)
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
	, patch_destination     (forge, map, lworld, LV2_PATCH__destination)
	, patch_property        (forge, map, lworld, LV2_PATCH__property)
	, patch_remove          (forge, map, lworld, LV2_PATCH__remove)
	, patch_request         (forge, map, lworld, LV2_PATCH__request)
	, patch_sequenceNumber  (forge, map, lworld, LV2_PATCH__sequenceNumber)
	, patch_subject         (forge, map, lworld, LV2_PATCH__subject)
	, patch_value           (forge, map, lworld, LV2_PATCH__value)
	, patch_wildcard        (forge, map, lworld, LV2_PATCH__wildcard)
	, pset_Preset           (forge, map, lworld, LV2_PRESETS__Preset)
	, pset_preset           (forge, map, lworld, LV2_PRESETS__preset)
	, pprops_logarithmic    (forge, map, lworld, LV2_PORT_PROPS__logarithmic)
	, rdf_type              (forge, map, lworld, NS_RDF "type")
	, rdfs_Class            (forge, map, lworld, NS_RDFS "Class")
	, rdfs_label            (forge, map, lworld, NS_RDFS "label")
	, rdfs_seeAlso          (forge, map, lworld, NS_RDFS "seeAlso")
	, rsz_minimumSize       (forge, map, lworld, LV2_RESIZE_PORT__minimumSize)
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

} // namespace Ingen
