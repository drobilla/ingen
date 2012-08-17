/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

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
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

namespace Ingen {

URIs::Quark::Quark(Forge& forge, URIMap* map, const char* c_str)
	: Raul::URI(c_str)
	, id(map->map_uri(c_str))
	, atom(forge.alloc_uri(c_str))
{
}

#define NS_INGEN "http://drobilla.net/ns/ingen#"
#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

URIs::URIs(Forge& f, URIMap* map)
	: forge(f)
	, atom_AtomPort         (forge, map, LV2_ATOM__AtomPort)
	, atom_Blank            (forge, map, LV2_ATOM__Blank)
	, atom_Bool             (forge, map, LV2_ATOM__Bool)
	, atom_Chunk            (forge, map, LV2_ATOM__Chunk)
	, atom_Float            (forge, map, LV2_ATOM__Float)
	, atom_Int              (forge, map, LV2_ATOM__Int)
	, atom_Resource         (forge, map, LV2_ATOM__Resource)
	, atom_Sequence         (forge, map, LV2_ATOM__Sequence)
	, atom_Sound            (forge, map, LV2_ATOM__Sound)
	, atom_String           (forge, map, LV2_ATOM__String)
	, atom_URI              (forge, map, LV2_ATOM__URI)
	, atom_URID             (forge, map, LV2_ATOM__URID)
	, atom_Vector           (forge, map, LV2_ATOM__Vector)
	, atom_bufferType       (forge, map, LV2_ATOM__bufferType)
	, atom_eventTransfer    (forge, map, LV2_ATOM__eventTransfer)
	, atom_supports         (forge, map, LV2_ATOM__supports)
	, doap_name             (forge, map, "http://usefulinc.com/ns/doap#name")
	, ingen_Edge            (forge, map, NS_INGEN "Edge")
	, ingen_Internal        (forge, map, NS_INGEN "Internal")
	, ingen_Node            (forge, map, NS_INGEN "Node")
	, ingen_Patch           (forge, map, NS_INGEN "Patch")
	, ingen_activity        (forge, map, NS_INGEN "activity")
	, ingen_broadcast       (forge, map, NS_INGEN "broadcast")
	, ingen_canvasX         (forge, map, NS_INGEN "canvasX")
	, ingen_canvasY         (forge, map, NS_INGEN "canvasY")
	, ingen_controlBinding  (forge, map, NS_INGEN "controlBinding")
	, ingen_document        (forge, map, NS_INGEN "document")
	, ingen_enabled         (forge, map, NS_INGEN "enabled")
	, ingen_engine          (forge, map, NS_INGEN "engine")
	, ingen_head            (forge, map, NS_INGEN "head")
	, ingen_incidentTo      (forge, map, NS_INGEN "incidentTo")
	, ingen_nil             (forge, map, NS_INGEN "nil")
	, ingen_node            (forge, map, NS_INGEN "node")
	, ingen_polyphonic      (forge, map, NS_INGEN "polyphonic")
	, ingen_polyphony       (forge, map, NS_INGEN "polyphony")
	, ingen_prototype       (forge, map, NS_INGEN "prototype")
	, ingen_sampleRate      (forge, map, NS_INGEN "sampleRate")
	, ingen_status          (forge, map, NS_INGEN "status")
	, ingen_tail            (forge, map, NS_INGEN "tail")
	, ingen_uiEmbedded      (forge, map, NS_INGEN "uiEmbedded")
	, ingen_value           (forge, map, NS_INGEN "value")
	, log_Error             (forge, map, LV2_LOG__Error)
	, log_Note              (forge, map, LV2_LOG__Note)
	, log_Warning           (forge, map, LV2_LOG__Warning)
	, lv2_AudioPort         (forge, map, LV2_CORE__AudioPort)
	, lv2_CVPort            (forge, map, LV2_CORE__CVPort)
	, lv2_ControlPort       (forge, map, LV2_CORE__ControlPort)
	, lv2_InputPort         (forge, map, LV2_CORE__InputPort)
	, lv2_OutputPort        (forge, map, LV2_CORE__OutputPort)
	, lv2_Plugin            (forge, map, LV2_CORE__Plugin)
	, lv2_connectionOptional(forge, map, LV2_CORE__connectionOptional)
	, lv2_default           (forge, map, LV2_CORE__default)
	, lv2_index             (forge, map, LV2_CORE__index)
	, lv2_integer           (forge, map, LV2_CORE__integer)
	, lv2_maximum           (forge, map, LV2_CORE__maximum)
	, lv2_minimum           (forge, map, LV2_CORE__minimum)
	, lv2_name              (forge, map, LV2_CORE__name)
	, lv2_portProperty      (forge, map, LV2_CORE__portProperty)
	, lv2_sampleRate        (forge, map, LV2_CORE__sampleRate)
	, lv2_scalePoint        (forge, map, LV2_CORE__scalePoint)
	, lv2_symbol            (forge, map, LV2_CORE__symbol)
	, lv2_toggled           (forge, map, LV2_CORE__toggled)
	, midi_Bender           (forge, map, LV2_MIDI__Bender)
	, midi_ChannelPressure  (forge, map, LV2_MIDI__ChannelPressure)
	, midi_Controller       (forge, map, LV2_MIDI__Controller)
	, midi_MidiEvent        (forge, map, LV2_MIDI__MidiEvent)
	, midi_NoteOn           (forge, map, LV2_MIDI__NoteOn)
	, midi_controllerNumber (forge, map, LV2_MIDI__controllerNumber)
	, midi_noteNumber       (forge, map, LV2_MIDI__noteNumber)
	, patch_Delete          (forge, map, LV2_PATCH__Delete)
	, patch_Get             (forge, map, LV2_PATCH__Get)
	, patch_Move            (forge, map, LV2_PATCH__Move)
	, patch_Patch           (forge, map, LV2_PATCH__Patch)
	, patch_Put             (forge, map, LV2_PATCH__Put)
	, patch_Response        (forge, map, LV2_PATCH__Response)
	, patch_Set             (forge, map, LV2_PATCH__Set)
	, patch_add             (forge, map, LV2_PATCH__add)
	, patch_body            (forge, map, LV2_PATCH__body)
	, patch_destination     (forge, map, LV2_PATCH__destination)
	, patch_remove          (forge, map, LV2_PATCH__remove)
	, patch_request         (forge, map, LV2_PATCH__request)
	, patch_subject         (forge, map, LV2_PATCH__subject)
	, pprops_logarithmic    (forge, map, LV2_PORT_PROPS__logarithmic)
	, rdf_type              (forge, map, NS_RDF "type")
	, rdfs_seeAlso          (forge, map, NS_RDFS "seeAlso")
	, wildcard              (forge, map, NS_INGEN "wildcard")
{
}

} // namespace Ingen
