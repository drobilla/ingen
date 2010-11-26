/* This file is part of Ingen.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

#ifndef INGEN_SHARED_LV2FEATURES_HPP
#define INGEN_SHARED_LV2FEATURES_HPP

#include <map>
#include <string>
#include <vector>
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "raul/SharedPtr.hpp"
#include "ingen-config.h"

namespace Ingen {
namespace Shared {

class Node;

/** Stuff that may need to be passed to an LV2 plugin (i.e. LV2 features).
 */
class LV2Features {
public:
	LV2Features();

	class Feature {
	public:
		virtual ~Feature() {}
		virtual SharedPtr<LV2_Feature> feature(Node* node) = 0;
	};

	class FeatureArray {
	public:
		typedef std::vector< SharedPtr<LV2_Feature> > FeatureVector;

		explicit FeatureArray(FeatureVector& features)
			: _features(features)
		{
			_array = (LV2_Feature**)malloc(sizeof(LV2_Feature) * (features.size() + 1));
			_array[features.size()] = NULL;
			for (size_t i = 0; i < features.size(); ++i)
				_array[i] = features[i].get();
		}

		~FeatureArray() {
			free(_array);
		}

		LV2_Feature** array() { return _array; }

	private:
		FeatureVector _features;
		LV2_Feature** _array;
	};

	SharedPtr<Feature> feature(const std::string& uri);

	void add_feature(const std::string& uri, SharedPtr<Feature> feature);

	SharedPtr<LV2Features::FeatureArray> lv2_features(Node* node) const;

private:
	typedef std::map< std::string, SharedPtr<Feature> > Features;
	Features _features;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2FEATURES_HPP
