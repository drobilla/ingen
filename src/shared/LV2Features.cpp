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

#include <cstdlib>

#include "LV2Features.hpp"
#include "LV2URIMap.hpp"

using namespace std;

namespace Ingen {
namespace Shared {

LV2Features::LV2Features()
{
}

void
LV2Features::add_feature(SharedPtr<Feature> feature)
{
	_features.push_back(feature);
}

LV2Features::FeatureArray::FeatureArray(FeatureVector& features)
	: _features(features)
{
	_array = (LV2_Feature**)malloc(sizeof(LV2_Feature) * (features.size() + 1));
	_array[features.size()] = NULL;
	for (size_t i = 0; i < features.size(); ++i) {
		_array[i] = features[i].get();
	}
}

LV2Features::FeatureArray::~FeatureArray()
{
	free(_array);
}

SharedPtr<LV2Features::FeatureArray>
LV2Features::lv2_features(Shared::World* world, Node* node) const
{
	FeatureArray::FeatureVector vec;
	for (Features::const_iterator f = _features.begin(); f != _features.end(); ++f) {
		SharedPtr<LV2_Feature> fptr = (*f)->feature(world, node);
		if (fptr) {
			vec.push_back(fptr);
		}
	}
	return SharedPtr<FeatureArray>(new FeatureArray(vec));
}

} // namespace Shared
} // namespace Ingen
