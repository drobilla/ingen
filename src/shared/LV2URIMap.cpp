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

#include "ingen/shared/LV2URIMap.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "raul/log.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {

LV2URIMap::LV2URIMap(LV2_URID_Map* map, LV2_URID_Unmap* unmap)
	: _uri_map_feature(new URIMapFeature(this))
	, _urid_map_feature(new URIDMapFeature(this, map))
	, _urid_unmap_feature(new URIDUnmapFeature(this, unmap))
{
}

LV2URIMap::URIMapFeature::URIMapFeature(LV2URIMap* map)
	: Feature(LV2_URI_MAP_URI, &_feature_data)
{
	_feature_data.uri_to_id     = &LV2URIMap::uri_map_uri_to_id;
	_feature_data.callback_data = map;
}

LV2URIMap::URIDMapFeature::URIDMapFeature(LV2URIMap*    map,
                                          LV2_URID_Map* urid_map)
	: Feature(LV2_URID__map, &_feature_data)
{
	if (urid_map) {
		_feature_data = *urid_map;
	} else {
		_feature_data.map    = default_map;
		_feature_data.handle = NULL;
	}
}

LV2_URID
LV2URIMap::URIDMapFeature::default_map(LV2_URID_Map_Handle handle,
                                       const char*         uri)
{
	return static_cast<LV2_URID>(g_quark_from_string(uri));
}

LV2_URID
LV2URIMap::URIDMapFeature::map(const char* uri)
{
	return _feature_data.map(_feature_data.handle, uri);
}


LV2URIMap::URIDUnmapFeature::URIDUnmapFeature(LV2URIMap*      map,
                                              LV2_URID_Unmap* urid_unmap)
	: Feature(LV2_URID__unmap, &_feature_data)
{
	if (urid_unmap) {
		_feature_data = *urid_unmap;
	} else {
		_feature_data.unmap  = default_unmap;
		_feature_data.handle = NULL;
	}
}

const char*
LV2URIMap::URIDUnmapFeature::default_unmap(LV2_URID_Unmap_Handle handle,
                                           LV2_URID              urid)
{
	return g_quark_to_string(urid);
}

const char*
LV2URIMap::URIDUnmapFeature::unmap(LV2_URID urid)
{
	return _feature_data.unmap(_feature_data.handle, urid);
}

uint32_t
LV2URIMap::uri_to_id(const char* map,
                     const char* uri)
{
	const uint32_t id = map_uri(uri);
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
		return unmap_uri(i->second);
	} else {
		return unmap_uri(id);
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

LV2_URID
LV2URIMap::urid_map(LV2_URID_Map_Handle handle, const char* uri)
{
	LV2URIMap* me = (LV2URIMap*)handle;
	return me->uri_to_id(NULL, uri);
}

uint32_t
LV2URIMap::map_uri(const char* uri)
{
	return _urid_map_feature->map(uri);
}

const char*
LV2URIMap::unmap_uri(uint32_t urid)
{
	return _urid_unmap_feature->unmap(urid);
}

} // namespace Shared
} // namespace Ingen
