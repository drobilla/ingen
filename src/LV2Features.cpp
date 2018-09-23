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

#include <cstdlib>

#include "ingen/LV2Features.hpp"

namespace ingen {

void
LV2Features::Feature::free_feature(LV2_Feature* feature)
{
	free(feature->data);
	free(feature);
}

LV2Features::LV2Features()
{
}

void
LV2Features::add_feature(SPtr<Feature> feature)
{
	_features.push_back(feature);
}

LV2Features::FeatureArray::FeatureArray(FeatureVector& features)
	: _features(features)
{
	_array = (LV2_Feature**)malloc(sizeof(LV2_Feature*) * (features.size() + 1));
	_array[features.size()] = nullptr;
	for (size_t i = 0; i < features.size(); ++i) {
		_array[i] = features[i].get();
	}
}

LV2Features::FeatureArray::~FeatureArray()
{
	free(_array);
}

bool
LV2Features::is_supported(const std::string& uri) const
{
	if (uri == "http://lv2plug.in/ns/lv2core#isLive") {
		return true;
	}

	for (const auto& f : _features) {
		if (f->uri() == uri) {
			return true;
		}
	}
	return false;
}

SPtr<LV2Features::FeatureArray>
LV2Features::lv2_features(World* world, Node* node) const
{
	FeatureArray::FeatureVector vec;
	for (const auto& f : _features) {
		SPtr<LV2_Feature> fptr = f->feature(world, node);
		if (fptr) {
			vec.push_back(fptr);
		}
	}
	return SPtr<FeatureArray>(new FeatureArray(vec));
}

} // namespace ingen
