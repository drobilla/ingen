/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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
#include "raul/log.hpp"
#include "object.lv2/object.h"
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


LV2URIMap::LV2URIMap()
	: ctx_context("ctx:context")
	, ctx_AudioContext("ctx:AudioContext")
	, ctx_MessageContext("ctx:MessageContext")
	, doap_name("doap:name")
	, ingen_LADSPAPlugin("ingen:LADSPAPlugin")
	, ingen_Internal("ingen:Internal")
	, ingen_Node("ingen:Node")
	, ingen_Patch("ingen:Patch")
	, ingen_Port("ingen:Port")
	, ingen_broadcast("ingen:broadcast")
	, ingen_enabled("ingen:enabled")
	, ingen_polyphonic("ingen:polyphonic")
	, ingen_polyphony("ingen:polyphony")
	, ingen_selected("ingen:selected")
	, ingen_value("ingen:value")
	, ingenui_canvas_x("ingenui:canvas-x")
	, ingenui_canvas_y("ingenui:canvas-y")
	, lv2_Plugin("lv2:Plugin")
	, lv2_index("lv2:index")
	, lv2_maximum("lv2:maximum")
	, lv2_minimum("lv2:minimum")
	, lv2_name("lv2:name")
	, lv2_symbol("lv2:symbol")
	, lv2_toggled("lv2:toggled")
	, midi_event("http://lv2plug.in/ns/ext/midi#MidiEvent")
	, object_class_bool(LV2_OBJECT_URI "#Bool")
	, object_class_float32(LV2_OBJECT_URI "#Float32")
	, object_class_int32(LV2_OBJECT_URI "#Int32")
	, object_class_string(LV2_OBJECT_URI "#String")
	, object_class_vector(LV2_OBJECT_URI "#Vector")
	, object_transfer(LV2_OBJECT_URI "#ObjectTransfer")
	, rdf_instanceOf("rdf:instanceOf")
	, rdf_type("rdf:type")
	, string_transfer("http://lv2plug.in/ns/dev/string-port#StringTransfer")
	, ui_format_events("http://lv2plug.in/ns/extensions/ui#Events")
{
	uri_map_feature_data.uri_to_id = &LV2URIMap::uri_map_uri_to_id;
	uri_map_feature_data.callback_data = this;
	uri_map_feature.URI = LV2_URI_MAP_URI;
	uri_map_feature.data = &uri_map_feature_data;
}


uint32_t
LV2URIMap::uri_to_id(const char* map,
                     const char* uri)
{
	const uint32_t ret = static_cast<uint32_t>(g_quark_from_string(uri));
	debug << "[LV2URIMap] ";
	if (map)
		debug << map << " : ";
	debug << uri << " => " << ret << endl;
	return ret;
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
