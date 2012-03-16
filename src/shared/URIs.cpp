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

URIs::Quark::Quark(LV2URIMap* map, const char* c_str)
	: Raul::URI(c_str)
	, id(map->map_uri(c_str))
{
}

#define NS_CTX   "http://lv2plug.in/ns/ext/contexts#"
#define NS_INGEN "http://drobilla.net/ns/ingen#"
#define NS_RDF   "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS  "http://www.w3.org/2000/01/rdf-schema#"

URIs::URIs(Raul::Forge& f, LV2URIMap* map)
	: forge(f)
	, atom_Bool             (map, LV2_ATOM__Bool)
	, atom_Float            (map, LV2_ATOM__Float)
	, atom_Int              (map, LV2_ATOM__Int)
	, atom_MessagePort      (map, LV2_ATOM__MessagePort)
	, atom_String           (map, LV2_ATOM__String)
	, atom_ValuePort        (map, LV2_ATOM__ValuePort)
	, atom_Vector           (map, LV2_ATOM__Vector)
	, atom_eventTransfer    (map, LV2_ATOM__eventTransfer)
	, atom_supports         (map, LV2_ATOM__supports)
	, ctx_audioContext      (map, NS_CTX "audioContext")
	, ctx_context           (map, NS_CTX "context")
	, ctx_messageContext    (map, NS_CTX "messageContext")
	, cv_CVPort             (map, "http://lv2plug.in/ns/ext/cv-port#CVPort")
	, doap_name             (map, "http://usefulinc.com/ns/doap#name")
	, ev_EventPort          (map, "http://lv2plug.in/ns/ext/event#EventPort")
	, ingen_Connection      (map, NS_INGEN "Connection")
	, ingen_Internal        (map, NS_INGEN "Internal")
	, ingen_Node            (map, NS_INGEN "Node")
	, ingen_Patch           (map, NS_INGEN "Patch")
	, ingen_Port            (map, NS_INGEN "Port")
	, ingen_activity        (map, NS_INGEN "activity")
	, ingen_broadcast       (map, NS_INGEN "broadcast")
	, ingen_canvasX         (map, NS_INGEN "canvasX")
	, ingen_canvasY         (map, NS_INGEN "canvasY")
	, ingen_controlBinding  (map, NS_INGEN "controlBinding")
	, ingen_destination     (map, NS_INGEN "destination")
	, ingen_document        (map, NS_INGEN "document")
	, ingen_enabled         (map, NS_INGEN "enabled")
	, ingen_engine          (map, NS_INGEN "engine")
	, ingen_nil             (map, NS_INGEN "nil")
	, ingen_node            (map, NS_INGEN "node")
	, ingen_polyphonic      (map, NS_INGEN "polyphonic")
	, ingen_polyphony       (map, NS_INGEN "polyphony")
	, ingen_sampleRate      (map, NS_INGEN "sampleRate")
	, ingen_selected        (map, NS_INGEN "selected")
	, ingen_source          (map, NS_INGEN "source")
	, ingen_value           (map, NS_INGEN "value")
	, lv2_AudioPort         (map, LV2_CORE__AudioPort)
	, lv2_ControlPort       (map, LV2_CORE__ControlPort)
	, lv2_InputPort         (map, LV2_CORE__InputPort)
	, lv2_OutputPort        (map, LV2_CORE__OutputPort)
	, lv2_Plugin            (map, LV2_CORE__Plugin)
	, lv2_connectionOptional(map, LV2_CORE__connectionOptional)
	, lv2_default           (map, LV2_CORE__default)
	, lv2_index             (map, LV2_CORE__index)
	, lv2_integer           (map, LV2_CORE__integer)
	, lv2_maximum           (map, LV2_CORE__maximum)
	, lv2_minimum           (map, LV2_CORE__minimum)
	, lv2_name              (map, LV2_CORE__name)
	, lv2_portProperty      (map, LV2_CORE__portProperty)
	, lv2_sampleRate        (map, LV2_CORE__sampleRate)
	, lv2_symbol            (map, LV2_CORE__symbol)
	, lv2_toggled           (map, LV2_CORE__toggled)
	, midi_Bender           (map, LV2_MIDI__Bender)
	, midi_ChannelPressure  (map, LV2_MIDI__ChannelPressure)
	, midi_Controller       (map, LV2_MIDI__Controller)
	, midi_MidiEvent        (map, LV2_MIDI__MidiEvent)
	, midi_NoteOn           (map, LV2_MIDI__NoteOn)
	, midi_controllerNumber (map, LV2_MIDI__controllerNumber)
	, midi_noteNumber       (map, LV2_MIDI__noteNumber)
	, patch_Get             (map, LV2_PATCH__Get)
	, patch_Put             (map, LV2_PATCH__Put)
	, patch_Response        (map, LV2_PATCH__Response)
	, patch_body            (map, LV2_PATCH__body)
	, patch_request         (map, LV2_PATCH__request)
	, patch_subject         (map, LV2_PATCH__subject)
	, rdf_instanceOf        (map, NS_RDF "instanceOf")
	, rdf_type              (map, NS_RDF "type")
	, rdfs_seeAlso          (map, NS_RDFS "seeAlso")
	, ui_Events             (map, "http://lv2plug.in/ns/extensions/ui#Events")
	, wildcard              (map, NS_INGEN "wildcard")
{
}

} // namespace Shared
} // namespace Ingen
