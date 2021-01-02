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

#ifndef INGEN_URIMAP_HPP
#define INGEN_URIMAP_HPP

#include "ingen/LV2Features.hpp"
#include "ingen/ingen.h"
#include "ingen/memory.hpp"
#include "lv2/core/lv2.h"
#include "lv2/urid/urid.h"
#include "raul/Noncopyable.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ingen {

class Log;
class Node;
class World;

/** URI to integer map and implementation of LV2 URID extension.
 * @ingroup IngenShared
 */
class INGEN_API URIMap : public raul::Noncopyable {
public:
	URIMap(Log& log, LV2_URID_Map* map, LV2_URID_Unmap* unmap);

	uint32_t    map_uri(const char* uri);
	uint32_t    map_uri(const std::string& uri) { return map_uri(uri.c_str()); }
	const char* unmap_uri(uint32_t urid) const;

	class Feature : public LV2Features::Feature {
	public:
		Feature(const char* URI, void* data) : _feature{URI, data} {}

		const char* uri() const override { return _feature.URI; }

		std::shared_ptr<LV2_Feature> feature(World&, Node*) override
		{
			return std::shared_ptr<LV2_Feature>(&_feature,
			                                    NullDeleter<LV2_Feature>);
		}

	private:
		LV2_Feature _feature;
	};

	struct URIDMapFeature : public Feature {
		URIDMapFeature(URIMap* map, LV2_URID_Map* impl, Log& log);

		LV2_URID        map(const char* uri);
		static LV2_URID default_map(LV2_URID_Map_Handle h, const char* c_uri);

		LV2_URID_Map&       data() { return _urid_map; }
		const LV2_URID_Map& data() const { return _urid_map; }

	private:
		LV2_URID_Map _urid_map;
		Log&         _log;
	};

	struct URIDUnmapFeature : public Feature {
		URIDUnmapFeature(URIMap* map, LV2_URID_Unmap* impl);

		const char*        unmap(LV2_URID urid) const;
		static const char* default_unmap(LV2_URID_Map_Handle h, LV2_URID urid);

		LV2_URID_Unmap&       data() { return _urid_unmap; }
		const LV2_URID_Unmap& data() const { return _urid_unmap; }

	private:
		LV2_URID_Unmap _urid_unmap;
	};

	const LV2_URID_Map&   urid_map() const { return _urid_map_feature->data(); }
	LV2_URID_Map&         urid_map() { return _urid_map_feature->data(); }
	const LV2_URID_Unmap& urid_unmap() const { return _urid_unmap_feature->data(); }
	LV2_URID_Unmap&       urid_unmap() { return _urid_unmap_feature->data(); }

	std::shared_ptr<URIDMapFeature> urid_map_feature()
	{
		return _urid_map_feature;
	}

	std::shared_ptr<URIDUnmapFeature> urid_unmap_feature()
	{
		return _urid_unmap_feature;
	}

private:
	friend struct URIDMapFeature;
	friend struct URIDUnMapFeature;

	std::shared_ptr<URIDMapFeature>   _urid_map_feature;
	std::shared_ptr<URIDUnmapFeature> _urid_unmap_feature;

	std::mutex                                _mutex;
	std::unordered_map<std::string, LV2_URID> _map;
	std::vector<std::string>                  _unmap;
};

} // namespace ingen

#endif // INGEN_URIMAP_HPP
