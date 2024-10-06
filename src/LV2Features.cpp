/*
  This file is part of Ingen.
  Copyright 2007-2024 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ingen/LV2Features.hpp"

#include "lv2/core/lv2.h"

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace ingen {

void
LV2Features::Feature::free_feature(LV2_Feature* feature)
{
	free(feature->data);
	free(feature);
}

void
LV2Features::add_feature(const std::shared_ptr<Feature>& feature)
{
	_features.push_back(feature);
}

LV2Features::FeatureArray::FeatureArray(FeatureVector& features)
    : _features(features)
    , _array{static_cast<LV2_Feature**>(
          calloc(features.size() + 1, sizeof(LV2_Feature*)))}
{
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

	return std::any_of(_features.begin(),
	                   _features.end(),
	                   [&uri](const auto& f) { return f->uri() == uri; });
}

std::shared_ptr<LV2Features::FeatureArray>
LV2Features::lv2_features(World& world, Node* node) const
{
	FeatureArray::FeatureVector vec;
	for (const auto& f : _features) {
		const std::shared_ptr<LV2_Feature> fptr = f->feature(world, node);
		if (fptr) {
			vec.push_back(fptr);
		}
	}

	return std::make_shared<FeatureArray>(vec);
}

} // namespace ingen
