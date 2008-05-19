/* This file is part of Ingen.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef LV2INFO_H
#define LV2INFO_H

#include CONFIG_H_PATH
#ifndef HAVE_SLV2
#error "This file requires SLV2, but HAVE_SLV2 is not defined.  Please report."
#endif

#include <map>
#include <string>
#include <slv2/slv2.h>
#include "module/global.hpp"
#include "lv2/uri_map/lv2_uri_map.h"
#include "lv2/event/lv2_event.h"

	
namespace Ingen {
	

/** Stuff that may need to be passed to an LV2 plugin (i.e. LV2 features).
 */
class LV2Info {
public:
	LV2Info(SLV2World world);
	~LV2Info();

	SLV2Value input_class;
	SLV2Value output_class;
	SLV2Value control_class;
	SLV2Value audio_class;
	SLV2Value event_class;

	LV2_Feature                     uri_map_feature;
	LV2_URI_Map_Feature             uri_map_feature_data;
	LV2_Feature                     event_feature;
	LV2_Event_Feature               event_feature_data;
	
	typedef std::map<std::string, uint32_t> URIMap;
	URIMap uri_map;
	uint32_t next_uri_id;

	static uint32_t uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
	                                  const char*               map,
	                                  const char*               uri);

	static uint32_t event_ref(LV2_Event_Callback_Data callback_data,
	                          LV2_Event*              event);

	LV2_Feature** lv2_features;
};


} // namespace Ingen

#endif // LV2INFO_H
