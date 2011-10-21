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

#include "raul/log.hpp"

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"

#include "LV2URIMap.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {

LV2URIMap::LV2URIMap(URIs& uris)
	: _uris(uris)
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
	const uint32_t id = static_cast<uint32_t>(g_quark_from_string(uri));
	if (map && !strcmp(map, LV2_EVENT_URI)) {
		GlobalToEvent::iterator i = _global_to_event.find(id);
		if (i != _global_to_event.end()) {
			return i->second;
		} else {
			if (_global_to_event.size() + 1 > UINT16_MAX) {
				error << "Event URI " << uri << " ID out of range." << endl;
				return 0;
			}
			const uint16_t ev_id = _global_to_event.size() + 1;
			assert(_event_to_global.find(ev_id) == _event_to_global.end());
			_global_to_event.insert(make_pair(id,    ev_id));
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
	if (map && !strcmp(map, LV2_EVENT_URI)) {
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

std::pair<bool, uint32_t>
LV2URIMap::event_to_global(uint16_t event_id) const
{
	EventToGlobal::const_iterator i = _event_to_global.find(event_id);
	if (i == _event_to_global.end()) {
		return std::make_pair(false, uint32_t(0));
	}
	return std::make_pair(true, i->second);
}

std::pair<bool, uint16_t>
LV2URIMap::global_to_event(uint32_t global_id) const
{
	GlobalToEvent::const_iterator i = _global_to_event.find(global_id);
	if (i == _global_to_event.end()) {
		return std::make_pair(false, uint16_t(0));
	}
	return std::make_pair(true, i->second);
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
