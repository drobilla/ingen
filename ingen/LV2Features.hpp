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

#ifndef INGEN_LV2FEATURES_HPP
#define INGEN_LV2FEATURES_HPP

#include <string>
#include <vector>

#include "ingen/ingen.h"
#include "ingen/types.hpp"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "raul/Noncopyable.hpp"

namespace Ingen {

class Node;
class World;

/** Features for use by LV2 plugins.
 * @ingroup IngenShared
 */
class INGEN_API LV2Features {
public:
	LV2Features();

	class Feature {
	public:
		virtual ~Feature() {}

		virtual const char* uri() const = 0;

		virtual SPtr<LV2_Feature> feature(World* world,
		                                  Node*  block) = 0;
	};

	class EmptyFeature : public Feature {
	public:
		EmptyFeature(const char* uri) : _uri(uri) {}

		virtual const char* uri() const { return _uri; }

		virtual SPtr<LV2_Feature> feature(World* world, Node* block) {
			return SPtr<LV2_Feature>();
		}

		const char* _uri;
	};

	class FeatureArray : public Raul::Noncopyable {
	public:
		typedef std::vector< SPtr<LV2_Feature> > FeatureVector;

		explicit FeatureArray(FeatureVector& features);

		~FeatureArray();

		LV2_Feature** array() { return _array; }

	private:
		FeatureVector _features;
		LV2_Feature** _array;
	};

	void add_feature(SPtr<Feature> feature);
	bool is_supported(const std::string& uri) const;

	SPtr<FeatureArray> lv2_features(World* world,
	                                Node*  block) const;

private:
	typedef std::vector< SPtr<Feature> > Features;
	Features _features;
};

} // namespace Ingen

#endif // INGEN_LV2FEATURES_HPP
