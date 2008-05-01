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

#define __STDC_LIMIT_MACROS 1
#include <cassert>
#include <iostream>
#include <stdint.h>
#include "LV2Info.hpp"

using namespace std;

namespace Ingen {
	
LV2Info::LV2Info(SLV2World world)
	: input_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_INPUT))
	, output_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_OUTPUT))
	, control_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_CONTROL))
	, audio_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_AUDIO))
	, event_class(slv2_value_new_uri(world, SLV2_PORT_CLASS_EVENT))
	, next_uri_id(1)
	, lv2_features(new LV2_Feature*[3])
{
	uri_map_feature_data.uri_to_id = &LV2Info::uri_map_uri_to_id;
	uri_map_feature_data.callback_data = this;
	uri_map_feature.URI = LV2_URI_MAP_URI;
	uri_map_feature.data = &uri_map_feature_data;

	event_feature_data.lv2_event_ref= &LV2Info::event_ref;
	event_feature_data.lv2_event_unref= &LV2Info::event_ref;
	event_feature_data.callback_data = this;
	event_feature.URI = LV2_EVENT_URI;
	event_feature.data = &event_feature_data;

	lv2_features[0] = &uri_map_feature;
	lv2_features[1] = &event_feature;
	lv2_features[2] = NULL;

	/* this is needed so we get a fixed type ID for MIDI, it would
	   probably be better to make the type map accessible from any
	   JackMidiPort. */
	next_uri_id++;
	uri_map.insert(make_pair(string("http://lv2plug.in/ns/ext/midi#MidiEvent"), 1));
}


LV2Info::~LV2Info()
{
	slv2_value_free(input_class);
	slv2_value_free(output_class);
	slv2_value_free(control_class);
	slv2_value_free(audio_class);
	slv2_value_free(event_class);
}


	
uint32_t
LV2Info::uri_map_uri_to_id(LV2_URI_Map_Callback_Data callback_data,
                           const char*               map,
                           const char*               uri)
{
	// TODO: map ignored, < UINT16_MAX assumed
	
	LV2Info* me = (LV2Info*)callback_data;
	uint32_t ret = 0;

	URIMap::iterator i = me->uri_map.find(uri);
	if (i != me->uri_map.end()) {
		ret = i->second;
	} else {
		ret = me->next_uri_id++;
		me->uri_map.insert(make_pair(string(uri), ret));
	}
	
	cout << "URI MAP (" << map << "): " << uri << " -> " << ret << endl; 

	assert(ret <= UINT16_MAX);
	return ret;
}


uint32_t 
LV2Info::event_ref(LV2_Event_Callback_Data callback_data,
		   LV2_Event*              event) {

}



} // namespace Ingen
