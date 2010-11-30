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
	: atom_AtomTransfer     (LV2_ATOM_URI "#AtomTransfer")
	, atom_Bool             (LV2_ATOM_URI "#Bool")
	, atom_Float32          (LV2_ATOM_URI "#Float32")
	, atom_Int32            (LV2_ATOM_URI "#Int32")
	, atom_MessagePort      (LV2_ATOM_URI "#MessagePort")
	, atom_String           (LV2_ATOM_URI "#String")
	, atom_ValuePort        (LV2_ATOM_URI "#ValuePort")
	, atom_Vector           (LV2_ATOM_URI "#Vector")
	, atom_supports         (LV2_ATOM_URI "#supports")
	, ctx_AudioContext      (NS_CTX "AudioContext")
	, ctx_MessageContext    (NS_CTX "MessageContext")
	, ctx_context           (NS_CTX "context")
	, doap_name             ("http://usefulinc.com/ns/doap#name")
	, ev_EventPort          ("http://lv2plug.in/ns/ext/event#EventPort")
	, ingen_Internal        (NS_INGEN "Internal")
	, ingen_LADSPAPlugin    (NS_INGEN "LADSPAPlugin")
	, ingen_Node            (NS_INGEN "Node")
	, ingen_Patch           (NS_INGEN "Patch")
	, ingen_Port            (NS_INGEN "Port")
	, ingen_broadcast       (NS_INGEN "broadcast")
	, ingen_controlBinding  (NS_INGEN "controlBinding")
	, ingen_document        (NS_INGEN "document")
	, ingen_enabled         (NS_INGEN "enabled")
	, ingen_nil             (NS_INGEN "nil")
	, ingen_node            (NS_INGEN "node")
	, ingen_polyphonic      (NS_INGEN "polyphonic")
	, ingen_polyphony       (NS_INGEN "polyphony")
	, ingen_selected        (NS_INGEN "selected")
	, ingen_value           (NS_INGEN "value")
	, ingenui_canvas_x      (NS_INGENUI "canvas-x")
	, ingenui_canvas_y      (NS_INGENUI "canvas-y")
	, lv2_AudioPort         (NS_LV2 "AudioPort")
	, lv2_ControlPort       (NS_LV2 "ControlPort")
	, lv2_InputPort         (NS_LV2 "InputPort")
	, lv2_OutputPort        (NS_LV2 "OutputPort")
	, lv2_Plugin            (NS_LV2 "Plugin")
	, lv2_default           (NS_LV2 "default")
	, lv2_index             (NS_LV2 "index")
	, lv2_integer           (NS_LV2 "integer")
	, lv2_maximum           (NS_LV2 "maximum")
	, lv2_minimum           (NS_LV2 "minimum")
	, lv2_name              (NS_LV2 "name")
	, lv2_portProperty      (NS_LV2 "portProperty")
	, lv2_symbol            (NS_LV2 "symbol")
	, lv2_toggled           (NS_LV2 "toggled")
	, midi_Bender           (NS_MIDI "Bender")
	, midi_ChannelPressure  (NS_MIDI "ChannelPressure")
	, midi_Controller       (NS_MIDI "Controller")
	, midi_MidiEvent        ("http://lv2plug.in/ns/ext/midi#MidiEvent")
	, midi_Note             (NS_MIDI "Note")
	, midi_controllerNumber (NS_MIDI "controllerNumber")
	, midi_noteNumber       (NS_MIDI "noteNumber")
	, rdf_instanceOf        (NS_RDF "instanceOf")
	, rdf_type              (NS_RDF "type")
	, rdfs_seeAlso          (NS_RDFS "seeAlso")
	, ui_Events             ("http://lv2plug.in/ns/extensions/ui#Events")
	, wildcard              (NS_INGEN "wildcard")
{
	uri_map_feature_data.uri_to_id = &LV2URIMap::uri_map_uri_to_id;
	uri_map_feature_data.callback_data = this;
	uri_map_feature.URI = LV2_URI_MAP_URI;
	uri_map_feature.data = &uri_map_feature_data;

	uri_unmap_feature_data.id_to_uri = &LV2URIMap::uri_unmap_id_to_uri;
	uri_unmap_feature_data.callback_data = this;
	uri_unmap_feature.URI = LV2_URI_UNMAP_URI;
	uri_unmap_feature.data = &uri_unmap_feature_data;

	_unmap_feature = SharedPtr<UnmapFeature>(new UnmapFeature(*this));
}


struct null_deleter { void operator()(void const *) const {} };


uint32_t
LV2URIMap::uri_to_id(const char* map,
                     const char* uri)
{
	const uint32_t id = static_cast<uint32_t>(g_quark_from_string(uri));
	if (map && !strcmp(map, "http://lv2plug.in/ns/ext/event")) {
		GlobalToEvent::iterator i = _global_to_event.find(id);
		if (i != _global_to_event.end()) {
			return i->second;
		} else {
			const uint16_t ev_id = _global_to_event.size() + 1;
			assert(_event_to_global.find(ev_id) == _event_to_global.end());
			_global_to_event.insert(make_pair(id, ev_id));
			_event_to_global.insert(make_pair(ev_id, id));
			return ev_id;
		}
	} else {
		return id;
	}
}


const char*
LV2URIMap::id_to_uri(const char*    map,
                     const uint32_t id)
{
	if (map && !strcmp(map, "http://lv2plug.in/ns/ext/event")) {
		EventToGlobal::iterator i = _event_to_global.find(id);
		if (i == _event_to_global.end()) {
			error << "Failed to unmap event URI " << id << endl;
			return NULL;
		}
		return g_quark_to_string(i->second);
	} else {
		return g_quark_to_string(id);
	}
}


uint32_t
LV2URIMap::uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
                             const char*               map,
                             const char*               uri)
{
	LV2URIMap* me = (LV2URIMap*)callback_data;
	return me->uri_to_id(map, uri);
}


const char*
LV2URIMap::uri_unmap_id_to_uri(LV2_URI_Map_Callback_Data callback_data,
                               const char*               map,
                               uint32_t                  id)
{
	LV2URIMap* me = (LV2URIMap*)callback_data;
	return me->id_to_uri(map, id);
}


} // namespace Shared
} // namespace Ingen
