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

#include "ingen/shared/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "raul/URI.hpp"

#include "LV2Features.hpp"

namespace Ingen {
namespace Shared {

/** Implementation of the LV2 URI Map extension
 */
class LV2URIMap : public boost::noncopyable, public LV2Features::Feature {
public:
	LV2URIMap(URIs& uris);

	SharedPtr<LV2_Feature> feature(Shared::World*, Node*) {
		return SharedPtr<LV2_Feature>(&uri_map_feature, NullDeleter<LV2_Feature>);
	}

	virtual uint32_t    uri_to_id(const char* map, const char* uri);
	virtual const char* id_to_uri(const char* map, uint32_t id);

	std::pair<bool, uint32_t> event_to_global(uint16_t event_id) const;
	std::pair<bool, uint16_t> global_to_event(uint32_t global_id) const;

private:
	static uint32_t uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
	                                  const char*               map,
	                                  const char*               uri);

	LV2_Feature         uri_map_feature;
	LV2_URI_Map_Feature uri_map_feature_data;

	typedef std::map<uint16_t, uint32_t> EventToGlobal;
	typedef std::map<uint32_t, uint16_t> GlobalToEvent;

	EventToGlobal _event_to_global;
	GlobalToEvent _global_to_event;

	URIs& _uris;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2URIMAP_HPP
