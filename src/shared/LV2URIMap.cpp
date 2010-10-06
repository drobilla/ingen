/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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
#include <cassert>
#include <stdint.h>
#include <glib.h>
#include <boost/shared_ptr.hpp>
#include "raul/log.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "LV2URIMap.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


LV2URIMap::Quark::Quark(const char* c_str)
	: Raul::URI(c_str)
	, id(g_quark_from_string(c_str))
{
}

const char*
LV2URIMap::Quark::c_str() const
{
	return g_quark_to_string(id);
}

#define NS_CTX     "http://lv2plug.in/ns/ext/contexts#"
#define NS_INGEN   "http://drobilla.net/ns/ingen#"
#define NS_INGENUI "http://drobilla.net/ns/ingenuity#"
#define NS_LV2     "http://lv2plug.in/ns/lv2core#"
#define NS_MIDI    "http://drobilla.net/ns/ext/midi#"
#define NS_MIDI    "http://drobilla.net/ns/ext/midi#"
#define NS_RDF     "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS    "http://www.w3.org/2000/01/rdf-schema#"

LV2URIMap::LV2URIMap()
	: ctx_AudioContext(NS_CTX "AudioContext")
	, ctx_MessageContext(NS_CTX "MessageContext")
	, ctx_context(NS_CTX "context")
	, doap_name("http://usefulinc.com/ns/doap#name")
	, ingen_Internal(NS_INGEN "Internal")
	, ingen_LADSPAPlugin(NS_INGEN "LADSPAPlugin")
	, ingen_Node(NS_INGEN "Node")
	, ingen_Patch(NS_INGEN "Patch")
	, ingen_Port(NS_INGEN "Port")
	, ingen_broadcast(NS_INGEN "broadcast")
	, ingen_controlBinding(NS_INGEN "controlBinding")
	, ingen_document(NS_INGEN "document")
	, ingen_enabled(NS_INGEN "enabled")
	, ingen_nil(NS_INGEN "nil")
	, ingen_node(NS_INGEN "node")
	, ingen_polyphonic(NS_INGEN "polyphonic")
	, ingen_polyphony(NS_INGEN "polyphony")
	, ingen_selected(NS_INGEN "selected")
	, ingen_value(NS_INGEN "value")
	, ingenui_canvas_x(NS_INGENUI "canvas-x")
	, ingenui_canvas_y(NS_INGENUI "canvas-y")
	, lv2_AudioPort(NS_LV2 "AudioPort")
	, lv2_ControlPort(NS_LV2 "ControlPort")
	, lv2_InputPort(NS_LV2 "InputPort")
	, lv2_OutputPort(NS_LV2 "OutputPort")
	, lv2_Plugin(NS_LV2 "Plugin")
	, lv2_default(NS_LV2 "default")
	, lv2_index(NS_LV2 "index")
	, lv2_integer(NS_LV2 "integer")
	, lv2_maximum(NS_LV2 "maximum")
	, lv2_minimum(NS_LV2 "minimum")
	, lv2_name(NS_LV2 "name")
	, lv2_portProperty(NS_LV2 "portProperty")
	, lv2_symbol(NS_LV2 "symbol")
	, lv2_toggled(NS_LV2 "toggled")
	, lv2ev_EventPort("http://lv2plug.in/ns/ext/event#EventPort")
	, midi_Bender(NS_MIDI "Bender")
	, midi_ChannelPressure(NS_MIDI "ChannelPressure")
	, midi_Controller(NS_MIDI "Controller")
	, midi_Note(NS_MIDI "Note")
	, midi_controllerNumber(NS_MIDI "controllerNumber")
	, midi_event("http://lv2plug.in/ns/ext/midi#MidiEvent")
	, midi_noteNumber(NS_MIDI "noteNumber")
	, obj_MessagePort("http://lv2plug.in/ns/ext/objects#MessagePort")
	, obj_ValuePort("http://lv2plug.in/ns/ext/objects#ValuePort")
	, obj_supports("http://lv2plug.in/ns/ext/objects#supports")
	, object_class_bool(LV2_ATOM_URI "#Bool")
	, object_class_float32(LV2_ATOM_URI "#Float32")
	, object_class_int32(LV2_ATOM_URI "#Int32")
	, object_class_string(LV2_ATOM_URI "#String")
	, object_class_vector(LV2_ATOM_URI "#Vector")
	, object_transfer(LV2_ATOM_URI "#ObjectTransfer")
	, rdf_instanceOf(NS_RDF "instanceOf")
	, rdf_type(NS_RDF "type")
	, rdfs_seeAlso(NS_RDFS "seeAlso")
	, string_transfer("http://lv2plug.in/ns/ext/string-port#StringTransfer")
	, ui_format_events("http://lv2plug.in/ns/extensions/ui#Events")
	, wildcard(NS_INGEN "wildcard")
{
	uri_map_feature_data.uri_to_id = &LV2URIMap::uri_map_uri_to_id;
	uri_map_feature_data.callback_data = this;
	uri_map_feature.URI = LV2_URI_MAP_URI;
	uri_map_feature.data = &uri_map_feature_data;
}


struct null_deleter { void operator()(void const *) const {} };


uint32_t
LV2URIMap::uri_to_id(const char* map,
                     const char* uri)
{
	return static_cast<uint32_t>(g_quark_from_string(uri));
}


uint32_t
LV2URIMap::uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
                             const char*               map,
                             const char*               uri)
{
	LV2URIMap* me = (LV2URIMap*)callback_data;
	return me->uri_to_id(map, uri);
}


} // namespace Shared
} // namespace Ingen
