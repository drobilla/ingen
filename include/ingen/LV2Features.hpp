/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "ingen/ingen.h"
#include "lv2/core/lv2.h"
#include "raul/Noncopyable.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ingen {

class Node;
class World;

/** Features for use by LV2 plugins.
 * @ingroup IngenShared
 */
class INGEN_API LV2Features {
public:
	LV2Features() = default;

	class Feature {
	public:
		virtual ~Feature() = default;

		virtual const char* uri() const = 0;

		virtual std::shared_ptr<LV2_Feature>
		feature(World& world, Node* block) = 0;

	protected:
		static void free_feature(LV2_Feature* feature);
	};

	class EmptyFeature : public Feature {
	public:
		explicit EmptyFeature(const char* uri) : _uri(uri) {}

		const char* uri() const override { return _uri; }

		std::shared_ptr<LV2_Feature> feature(World& world, Node* block) override
		{
			return nullptr;
		}

		const char* _uri;
	};

	class FeatureArray : public raul::Noncopyable {
	public:
		using FeatureVector = std::vector<std::shared_ptr<LV2_Feature>>;

		explicit FeatureArray(FeatureVector& features);

		~FeatureArray();

		LV2_Feature** array() { return _array; }

	private:
		FeatureVector _features;
		LV2_Feature** _array;
	};

	void add_feature(const std::shared_ptr<Feature>& feature);
	bool is_supported(const std::string& uri) const;

	std::shared_ptr<FeatureArray> lv2_features(World& world, Node*  node) const;

private:
	using Features = std::vector<std::shared_ptr<Feature>>;
	Features _features;
};

} // namespace ingen

#endif // INGEN_LV2FEATURES_HPP
