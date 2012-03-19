/* This file is part of Ingen.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define __STDC_LIMIT_MACROS 1

#include <assert.h>
#include <stdint.h>

#include <glib.h>

#include <boost/shared_ptr.hpp>

#include "ingen/shared/URIs.hpp"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "raul/log.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {

URIs::Quark::Quark(Ingen::Forge& forge, LV2URIMap* map, const char* c_str)
	: Raul::URI(c_str)
	, id(map->map_uri(c_str))
	, atom(forge.alloc_uri(c_str))
{
}

#define NS_CTX   "http://lv2plug.in/ns/ext/contexts#"
#define NS_INGEN "http://drobilla.net/ns/ingen#"
#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

URIs::URIs(Ingen::Forge& f, LV2URIMap* map)
	: forge(f)
	, atom_Blank            (forge, map, LV2_ATOM__Blank)
	, atom_Bool             (forge, map, LV2_ATOM__Bool)
	, atom_Float            (forge, map, LV2_ATOM__Float)
	, atom_Int              (forge, map, LV2_ATOM__Int)
	, atom_MessagePort      (forge, map, LV2_ATOM__MessagePort)
	, atom_Sequence         (forge, map, LV2_ATOM__Sequence)
	, atom_Sound            (forge, map, LV2_ATOM__Sound)
	, atom_String           (forge, map, LV2_ATOM__String)
	, atom_ValuePort        (forge, map, LV2_ATOM__ValuePort)
	, atom_Vector           (forge, map, LV2_ATOM__Vector)
	, atom_bufferType       (forge, map, LV2_ATOM__bufferType)
	, atom_eventTransfer    (forge, map, LV2_ATOM__eventTransfer)
	, atom_supports         (forge, map, LV2_ATOM__supports)
	, ctx_audioContext      (forge, map, NS_CTX "audioContext")
	, ctx_context           (forge, map, NS_CTX "context")
	, ctx_messageContext    (forge, map, NS_CTX "messageContext")
	, cv_CVPort             (forge, map, "http://lv2plug.in/ns/ext/cv-port#CVPort")
	, doap_name             (forge, map, "http://usefulinc.com/ns/doap#name")
	, ingen_Connection      (forge, map, NS_INGEN "Connection")
	, ingen_Internal        (forge, map, NS_INGEN "Internal")
	, ingen_Node            (forge, map, NS_INGEN "Node")
	, ingen_Patch           (forge, map, NS_INGEN "Patch")
	, ingen_Port            (forge, map, NS_INGEN "Port")
	, ingen_activity        (forge, map, NS_INGEN "activity")
	, ingen_broadcast       (forge, map, NS_INGEN "broadcast")
	, ingen_canvasX         (forge, map, NS_INGEN "canvasX")
	, ingen_canvasY         (forge, map, NS_INGEN "canvasY")
	, ingen_controlBinding  (forge, map, NS_INGEN "controlBinding")
	, ingen_destination     (forge, map, NS_INGEN "destination")
	, ingen_document        (forge, map, NS_INGEN "document")
	, ingen_enabled         (forge, map, NS_INGEN "enabled")
	, ingen_engine          (forge, map, NS_INGEN "engine")
	, ingen_nil             (forge, map, NS_INGEN "nil")
	, ingen_node            (forge, map, NS_INGEN "node")
	, ingen_polyphonic      (forge, map, NS_INGEN "polyphonic")
	, ingen_polyphony       (forge, map, NS_INGEN "polyphony")
	, ingen_sampleRate      (forge, map, NS_INGEN "sampleRate")
	, ingen_selected        (forge, map, NS_INGEN "selected")
	, ingen_source          (forge, map, NS_INGEN "source")
	, ingen_value           (forge, map, NS_INGEN "value")
	, lv2_AudioPort         (forge, map, LV2_CORE__AudioPort)
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
	, lv2_symbol            (forge, map, LV2_CORE__symbol)
	, lv2_toggled           (forge, map, LV2_CORE__toggled)
	, midi_Bender           (forge, map, LV2_MIDI__Bender)
	, midi_ChannelPressure  (forge, map, LV2_MIDI__ChannelPressure)
	, midi_Controller       (forge, map, LV2_MIDI__Controller)
	, midi_MidiEvent        (forge, map, LV2_MIDI__MidiEvent)
	, midi_NoteOn           (forge, map, LV2_MIDI__NoteOn)
	, midi_controllerNumber (forge, map, LV2_MIDI__controllerNumber)
	, midi_noteNumber       (forge, map, LV2_MIDI__noteNumber)
	, patch_Get             (forge, map, LV2_PATCH__Get)
	, patch_Put             (forge, map, LV2_PATCH__Put)
	, patch_Response        (forge, map, LV2_PATCH__Response)
	, patch_body            (forge, map, LV2_PATCH__body)
	, patch_request         (forge, map, LV2_PATCH__request)
	, patch_subject         (forge, map, LV2_PATCH__subject)
	, rdf_instanceOf        (forge, map, NS_RDF "instanceOf")
	, rdf_type              (forge, map, NS_RDF "type")
	, rdfs_seeAlso          (forge, map, NS_RDFS "seeAlso")
	, ui_Events             (forge, map, "http://lv2plug.in/ns/extensions/ui#Events")
	, wildcard              (forge, map, NS_INGEN "wildcard")
{
}

} // namespace Shared
} // namespace Ingen
