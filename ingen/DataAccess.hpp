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

#ifndef INGEN_ENGINE_DATAACCESS_HPP
#define INGEN_ENGINE_DATAACCESS_HPP

#include <cstdlib>
#include <utility>

#include "ingen/LV2Features.hpp"
#include "ingen/Node.hpp"
#include "ingen/Store.hpp"
#include "ingen/World.hpp"
#include "ingen/types.hpp"
#include "lilv/lilv.h"

#include "lv2/core/lv2.h"
#include "lv2/data-access/data-access.h"

namespace ingen {

struct DataAccess : public ingen::LV2Features::Feature
{
	static void delete_feature(LV2_Feature* feature) {
		free(feature->data);
		delete feature;
	}

	const char* uri() const override { return "http://lv2plug.in/ns/ext/data-access"; }

	SPtr<LV2_Feature> feature(World& world, Node* node) override {
		Node* store_node = world.store()->get(node->path());
		if (!store_node) {
			return SPtr<LV2_Feature>();
		}

		LilvInstance* inst = store_node->instance();
		if (!inst) {
			return SPtr<LV2_Feature>();
		}

		const LV2_Descriptor*       desc = lilv_instance_get_descriptor(inst);
		LV2_Extension_Data_Feature* data = (LV2_Extension_Data_Feature*)
			malloc(sizeof(LV2_Extension_Data_Feature));

		data->data_access = desc->extension_data;

		return SPtr<LV2_Feature>(
			new LV2_Feature{ "http://lv2plug.in/ns/ext/data-access", data });
	}
};

} // namespace ingen

#endif // INGEN_ENGINE_DATAACCESS_HPP
