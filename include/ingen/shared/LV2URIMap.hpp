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
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
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

	class URIMapFeature : public Feature {
	public:
		URIMapFeature(LV2URIMap* map);

	private:
		LV2_URI_Map_Feature _feature_data;
	};

	class URIDMapFeature : public Feature {
	public:
		URIDMapFeature(LV2URIMap* map, LV2_URID_Map* urid_map);
	private:
		LV2_URID_Map _feature_data;
	};

	class URIDUnmapFeature : public Feature {
	public:
		URIDUnmapFeature(LV2URIMap* map, LV2_URID_Unmap* urid_unmap);
	private:
		LV2_URID_Unmap _feature_data;
	};

	SharedPtr<URIMapFeature>    uri_map_feature()    { return _uri_map_feature; }
	SharedPtr<URIDMapFeature>   urid_map_feature()   { return _urid_map_feature; }
	SharedPtr<URIDUnmapFeature> urid_unmap_feature() { return _urid_unmap_feature; }

	virtual uint32_t    uri_to_id(const char* map, const char* uri);
	virtual const char* id_to_uri(const char* map, uint32_t id);

	std::pair<bool, uint32_t> event_to_global(uint16_t event_id) const;
	std::pair<bool, uint16_t> global_to_event(uint32_t global_id) const;

private:
	static uint32_t uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
	                                  const char*               map,
	                                  const char*               uri);

	static LV2_URID    urid_map(LV2_URID_Map_Handle handle, const char* uri);
	static const char* urid_unmap(LV2_URID_Map_Handle handle, LV2_URID urid);

	typedef std::map<uint16_t, uint32_t> EventToGlobal;
	typedef std::map<uint32_t, uint16_t> GlobalToEvent;

	SharedPtr<URIMapFeature>    _uri_map_feature;
	SharedPtr<URIDMapFeature>   _urid_map_feature;
	SharedPtr<URIDUnmapFeature> _urid_unmap_feature;
	EventToGlobal               _event_to_global;
	GlobalToEvent               _global_to_event;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2URIMAP_HPP
