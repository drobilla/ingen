/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_SHARED_LV2FEATURES_HPP
#define INGEN_SHARED_LV2FEATURES_HPP

#include <vector>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "raul/SharedPtr.hpp"

namespace Ingen {

class Node;

namespace Shared {

class World;

/** Stuff that may need to be passed to an LV2 plugin (i.e. LV2 features).
 */
class LV2Features {
public:
	LV2Features();

	class Feature {
	public:
		virtual ~Feature() {}

		virtual SharedPtr<LV2_Feature> feature(Shared::World* world,
		                                       Node*          node) = 0;
	};

	class FeatureArray {
	public:
		typedef std::vector< SharedPtr<LV2_Feature> > FeatureVector;

		explicit FeatureArray(FeatureVector& features);

		~FeatureArray();

		LV2_Feature** array() { return _array; }

	private:
		FeatureVector _features;
		LV2_Feature** _array;
	};

	void add_feature(SharedPtr<Feature> feature);

	SharedPtr<FeatureArray> lv2_features(Shared::World* world,
	                                     Node*          node) const;

private:
	typedef std::vector< SharedPtr<Feature> > Features;
	Features _features;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_LV2FEATURES_HPP