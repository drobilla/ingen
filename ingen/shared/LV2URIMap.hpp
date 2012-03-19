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

#ifndef INGEN_SHARED_LV2URIMAP_HPP
#define INGEN_SHARED_LV2URIMAP_HPP

#include <map>
#include <utility>

#include <boost/utility.hpp>

#include "ingen/shared/LV2Features.hpp"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/URI.hpp"

namespace Ingen {
namespace Shared {

/** URI to Integer Map */
class LV2URIMap : public boost::noncopyable {
public:
	LV2URIMap(LV2_URID_Map* map, LV2_URID_Unmap* unmap);
	virtual ~LV2URIMap() {}

	uint32_t    map_uri(const char* uri);
	const char* unmap_uri(uint32_t urid);

	class Feature : public LV2Features::Feature {
	public:
		Feature(const char* URI, void* data) {
			_feature.URI  = URI;
			_feature.data = data;
		}

		SharedPtr<LV2_Feature> feature(Shared::World*, Node*) {
			return SharedPtr<LV2_Feature>(&_feature, NullDeleter<LV2_Feature>);
		}

	private:
		LV2_Feature _feature;
	};

	struct URIDMapFeature : public Feature {
		URIDMapFeature(LV2URIMap* map, LV2_URID_Map* urid_map);
		LV2_URID        map(const char* uri);
		static LV2_URID default_map(LV2_URID_Map_Handle h, const char* uri);
		LV2_URID_Map urid_map;
	};

	struct URIDUnmapFeature : public Feature {
		URIDUnmapFeature(LV2URIMap* map, LV2_URID_Unmap* urid_unmap);
		const char*        unmap(const LV2_URID urid);
		static const char* default_unmap(LV2_URID_Map_Handle h, LV2_URID uri);
		LV2_URID_Unmap urid_unmap;
	};

	SharedPtr<URIDMapFeature>   urid_map_feature()   { return _urid_map_feature; }
	SharedPtr<URIDUnmapFeature> urid_unmap_feature() { return _urid_unmap_feature; }

private:
	SharedPtr<URIDMapFeature>   _urid_map_feature;
	SharedPtr<URIDUnmapFeature> _urid_unmap_feature;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2URIMAP_HPP
