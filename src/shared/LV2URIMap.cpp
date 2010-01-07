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
#include "raul/log.hpp"
#include "object.lv2/object.h"
#include "LV2URIMap.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {


LV2URIMap::LV2URIMap()
	: uri_map()
	, next_uri_id(1)
	, object_class_bool(uri_to_id(NULL, LV2_OBJECT_URI "#Bool"))
	, object_class_string(uri_to_id(NULL, LV2_OBJECT_URI "#String"))
	, object_class_int32(uri_to_id(NULL, LV2_OBJECT_URI "#Int32"))
	, object_class_float32(uri_to_id(NULL, LV2_OBJECT_URI "#Float32"))
	, object_class_vector(uri_to_id(NULL, LV2_OBJECT_URI "#Vector"))
	, ui_format_events(uri_to_id(NULL, "http://lv2plug.in/ns/extensions/ui#Events"))
	, midi_event(uri_to_id(NULL, "http://lv2plug.in/ns/ext/midi#MidiEvent"))
	, string_transfer(uri_to_id(NULL, "http://lv2plug.in/ns/dev/string-port#StringTransfer"))
	, object_transfer(uri_to_id(NULL, LV2_OBJECT_URI "#ObjectTransfer"))
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
	return uri_map_uri_to_id(this, map, uri);
}


uint32_t
LV2URIMap::uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
                             const char*               map,
                             const char*               uri)
{
	// TODO: map ignored, < UINT16_MAX assumed

	LV2URIMap* me = (LV2URIMap*)callback_data;
	uint32_t ret = 0;

	URIMap::iterator i = me->uri_map.find(uri);
	if (i != me->uri_map.end()) {
		ret = i->second;
	} else {
		ret = me->next_uri_id++;
		me->uri_map.insert(make_pair(string(uri), ret));
	}

	debug << "[LV2URIMap] ";
	if (map)
		debug << map << " : ";
	debug << uri << " => " << ret << endl;

	assert(ret <= UINT16_MAX);
	return ret;
}


} // namespace Shared
} // namespace Ingen
