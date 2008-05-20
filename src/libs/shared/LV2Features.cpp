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

#include <cstdlib>
#include "LV2Features.hpp"
#include "LV2URIMap.hpp"

using namespace std;

namespace Ingen {
namespace Shared {
	
	
LV2Features::LV2Features()
	: _lv2_features((LV2_Feature**)malloc(sizeof(LV2_Feature*)))
{
	_lv2_features[0] = NULL;

	LV2URIMap* controller = new LV2URIMap();
	add_feature(LV2_URI_MAP_URI, &controller->uri_map_feature, controller);
}


const LV2Features::Feature*
LV2Features::feature(const std::string& uri)
{
	Features::const_iterator i = _features.find(uri);
	if (i != _features.end())
		return &i->second;
	else
		return NULL;
}


void
LV2Features::add_feature(const std::string& uri, LV2_Feature* feature, void* controller)
{
#ifndef NDEBUG
	Features::const_iterator i = _features.find(uri);
	assert(i == _features.end());
	assert(_lv2_features[_features.size()] == NULL);
#endif
	_features.insert(make_pair(uri, Feature(feature, controller)));

	_lv2_features = (LV2_Feature**)realloc(_lv2_features, _features.size() + 1);
	_lv2_features[_features.size()-1] = feature;
	_lv2_features[_features.size()] = NULL;
}
	

} // namespace Shared
} // namespace Ingen
