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

#ifndef INGEN_URIMAP_HPP
#define INGEN_URIMAP_HPP

#include <map>
#include <utility>

#include "ingen/LV2Features.hpp"
#include "ingen/Log.hpp"
#include "ingen/ingen.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/Noncopyable.hpp"
#include "raul/URI.hpp"

namespace Ingen {

/** URI to integer map and implementation of LV2 URID extension.
 * @ingroup IngenShared
 */
class INGEN_API URIMap : public Raul::Noncopyable {
public:
	URIMap(Log& log, LV2_URID_Map* map, LV2_URID_Unmap* unmap);
	virtual ~URIMap() {}

	uint32_t    map_uri(const char* uri);
	const char* unmap_uri(uint32_t urid) const;

	class Feature : public LV2Features::Feature {
	public:
		Feature(const char* URI, void* data) {
			_feature.URI  = URI;
			_feature.data = data;
		}

		const char* uri() const { return _feature.URI; }

		SPtr<LV2_Feature> feature(World*, Node*) {
			return SPtr<LV2_Feature>(&_feature, NullDeleter<LV2_Feature>);
		}

	private:
		LV2_Feature _feature;
	};

	struct URIDMapFeature : public Feature {
		URIDMapFeature(URIMap* map, LV2_URID_Map* urid_map, Log& log);
		LV2_URID        map(const char* uri);
		static LV2_URID default_map(LV2_URID_Map_Handle h, const char* uri);
		LV2_URID_Map urid_map;
		Log&         log;
	};

	struct URIDUnmapFeature : public Feature {
		URIDUnmapFeature(URIMap* map, LV2_URID_Unmap* urid_unmap);
		const char*        unmap(const LV2_URID urid);
		static const char* default_unmap(LV2_URID_Map_Handle h, LV2_URID uri);
		LV2_URID_Unmap urid_unmap;
	};

	SPtr<URIDMapFeature>   urid_map_feature()   { return _urid_map_feature; }
	SPtr<URIDUnmapFeature> urid_unmap_feature() { return _urid_unmap_feature; }

private:
	SPtr<URIDMapFeature>   _urid_map_feature;
	SPtr<URIDUnmapFeature> _urid_unmap_feature;
};

} // namespace Ingen

#endif // INGEN_URIMAP_HPP
