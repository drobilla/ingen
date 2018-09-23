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

#ifndef INGEN_ENGINE_INSTANCEACCESS_HPP
#define INGEN_ENGINE_INSTANCEACCESS_HPP

#include <utility>

#include "ingen/LV2Features.hpp"
#include "ingen/Node.hpp"
#include "ingen/Store.hpp"
#include "ingen/World.hpp"
#include "ingen/types.hpp"
#include "lilv/lilv.h"

#include "lv2/core/lv2.h"

namespace ingen {

struct InstanceAccess : public ingen::LV2Features::Feature
{
	const char* uri() const override { return "http://lv2plug.in/ns/ext/instance-access"; }

	SPtr<LV2_Feature> feature(World* world, Node* node) override {
		Node* store_node = world->store()->get(node->path());
		if (!store_node) {
			return SPtr<LV2_Feature>();
		}

		LilvInstance* instance = store_node->instance();
		if (!instance) {
			return SPtr<LV2_Feature>();
		}

		return SPtr<LV2_Feature>(
			new LV2_Feature{ "http://lv2plug.in/ns/ext/instance-access",
			                 lilv_instance_get_handle(instance) });
	}
};

} // namespace ingen

#endif // INGEN_ENGINE_INSTANCEACCESS_HPP
