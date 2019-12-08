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

#include "ingen/Log.hpp"
#include "ingen/URI.hpp"
#include "lv2/urid/urid.h"

#include <cassert>
#include <cstdint>
#include <utility>

namespace ingen {

URIMap::URIMap(Log& log, LV2_URID_Map* map, LV2_URID_Unmap* unmap)
	: _urid_map_feature(new URIDMapFeature(this, map, log))
	, _urid_unmap_feature(new URIDUnmapFeature(this, unmap))
{
}

URIMap::URIDMapFeature::URIDMapFeature(URIMap*       map,
                                       LV2_URID_Map* impl,
                                       Log&          log)
	: Feature(LV2_URID__map, &urid_map)
	, urid_map()
	, log(log)
{
	if (impl) {
		urid_map = *impl;
	} else {
		urid_map.map    = default_map;
		urid_map.handle = map;
	}
}

LV2_URID
URIMap::URIDMapFeature::default_map(LV2_URID_Map_Handle h,
                                    const char*         c_uri)
{
	auto* const                 map((URIMap*)h);
	std::string                 uri(c_uri);
	std::lock_guard<std::mutex> lock(map->_mutex);

	auto       record = map->_map.emplace(uri, map->_map.size() + 1);
	const auto id     = record.first->second;
	if (record.second) {
		assert(id == map->_map.size());
		assert(id == map->_unmap.size() + 1);
		map->_unmap.emplace_back(std::move(uri));
	}
	return id;
}

LV2_URID
URIMap::URIDMapFeature::map(const char* uri)
{
	if (!URI::is_valid(uri)) {
		log.error("Attempt to map invalid URI <%1%>\n", uri);
		return 0;
	}
	return urid_map.map(urid_map.handle, uri);
}

URIMap::URIDUnmapFeature::URIDUnmapFeature(URIMap*         map,
                                           LV2_URID_Unmap* impl)
	: Feature(LV2_URID__unmap, &urid_unmap)
	, urid_unmap()
{
	if (impl) {
		urid_unmap = *impl;
	} else {
		urid_unmap.unmap  = default_unmap;
		urid_unmap.handle = map;
	}
}

const char*
URIMap::URIDUnmapFeature::default_unmap(LV2_URID_Unmap_Handle h,
                                        LV2_URID              urid)
{
	auto* const                 map((URIMap*)h);
	std::lock_guard<std::mutex> lock(map->_mutex);

	return (urid > 0 && urid <= map->_unmap.size()
	        ? map->_unmap[urid - 1].c_str()
	        : nullptr);
}

const char*
URIMap::URIDUnmapFeature::unmap(LV2_URID urid)
{
	return urid_unmap.unmap(urid_unmap.handle, urid);
}

uint32_t
URIMap::map_uri(const char* uri)
{
	const uint32_t urid = _urid_map_feature->map(uri);
#ifdef INGEN_DEBUG_URIDS
	fprintf(stderr, "Map URI %3u <= %s\n", urid, uri);
#endif
	return urid;
}

const char*
URIMap::unmap_uri(uint32_t urid) const
{
	const char* uri = _urid_unmap_feature->unmap(urid);
#ifdef INGEN_DEBUG_URIDS
	fprintf(stderr, "Unmap URI %3u => %s\n", urid, uri);
#endif
	return uri;
}

} // namespace ingen
