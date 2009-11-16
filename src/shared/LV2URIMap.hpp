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

#ifndef LV2URIMAP_HPP
#define LV2URIMAP_HPP

#include "ingen-config.h"
#ifndef HAVE_SLV2
#error "This file requires SLV2, but HAVE_SLV2 is not defined.  Please report."
#endif

#include <map>
#include <string>
#include <boost/utility.hpp>
#include "slv2/slv2.h"
#include "uri-map.lv2/uri-map.h"
#include "LV2Features.hpp"

namespace Ingen {
namespace Shared {


/** Implementation of the LV2 URI Map extension
 */
class LV2URIMap : public boost::noncopyable, public LV2Features::Feature {
public:
	LV2URIMap();

	SharedPtr<LV2_Feature> feature(Node*) {
		return SharedPtr<LV2_Feature>(&uri_map_feature, NullDeleter<LV2_Feature>);
	}

	uint32_t uri_to_id(const char* map, const char* uri);

private:
	typedef std::map<std::string, uint32_t> URIMap;

	static uint32_t uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
	                                  const char*               map,
	                                  const char*               uri);

	LV2_Feature         uri_map_feature;
	LV2_URI_Map_Feature uri_map_feature_data;
	URIMap              uri_map;
	uint32_t            next_uri_id;

public:
	const uint32_t object_class_bool;
	const uint32_t object_class_string;
	const uint32_t object_class_int32;
	const uint32_t object_class_float32;
	const uint32_t object_class_vector;
	const uint32_t ui_format_events;
	const uint32_t midi_event;
	const uint32_t string_transfer;
	const uint32_t object_transfer;
};


} // namespace Shared
} // namespace Ingen

#endif // LV2URIMAP_HPP
