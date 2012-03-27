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

#define __STDC_LIMIT_MACROS 1

#include <assert.h>
#include <stdint.h>

#include <glib.h>

#include <boost/shared_ptr.hpp>

#include "ingen/shared/LV2URIMap.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "raul/log.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Shared {

LV2URIMap::LV2URIMap(LV2_URID_Map* map, LV2_URID_Unmap* unmap)
	: _urid_map_feature(new URIDMapFeature(this, map))
	, _urid_unmap_feature(new URIDUnmapFeature(this, unmap))
{
}

LV2URIMap::URIDMapFeature::URIDMapFeature(LV2URIMap*    map,
                                          LV2_URID_Map* impl)
	: Feature(LV2_URID__map, &urid_map)
{
	if (impl) {
		urid_map = *impl;
	} else {
		urid_map.map    = default_map;
		urid_map.handle = NULL;
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
	return urid_map.map(urid_map.handle, uri);
}

LV2URIMap::URIDUnmapFeature::URIDUnmapFeature(LV2URIMap*      map,
                                              LV2_URID_Unmap* impl)
	: Feature(LV2_URID__unmap, &urid_unmap)
{
	if (impl) {
		urid_unmap = *impl;
	} else {
		urid_unmap.unmap  = default_unmap;
		urid_unmap.handle = NULL;
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
	return urid_unmap.unmap(urid_unmap.handle, urid);
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
