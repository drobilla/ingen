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

#ifndef INGEN_ENGINE_LV2_REQUEST_RUN_FEATURE_HPP
#define INGEN_ENGINE_LV2_REQUEST_RUN_FEATURE_HPP

#include "lv2/lv2plug.in/ns/ext/worker/worker.h"

#include "raul/log.hpp"

#include "ingen/shared/LV2Features.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "MessageContext.hpp"
#include "NodeImpl.hpp"
#include "PortImpl.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Server {

struct RequestRunFeature : public Ingen::Shared::LV2Features::Feature {
	struct Info {
		inline Info(Shared::World* w, Node* n) : world(w), node(n) {}
		Shared::World* world;
		Node*          node;
	};

	static LV2_Worker_Status
	schedule_work(LV2_Worker_Schedule_Handle handle,
	              uint32_t                   size,
	              const void*                data)
	{
		Info* info = reinterpret_cast<Info*>(handle);
		if (!info->world->local_engine())
			return LV2_WORKER_ERR_UNKNOWN;

		Engine* engine = (Engine*)info->world->local_engine().get();
		engine->message_context()->run(
			dynamic_cast<NodeImpl*>(info->node),
			engine->driver()->frame_time());

		return LV2_WORKER_SUCCESS;
	}

	static void delete_feature(LV2_Feature* feature) {
		free(feature->data);
		free(feature);
	}

	SharedPtr<LV2_Feature> feature(Shared::World* world, Node* n) {
		const NodeImpl* node = dynamic_cast<const NodeImpl*>(n);
		if (!node)
			return SharedPtr<LV2_Feature>();

		LV2_Worker_Schedule* data = (LV2_Worker_Schedule*)malloc(
			sizeof(LV2_Worker_Schedule));
		data->handle        = new Info(world, n);
		data->schedule_work = schedule_work;

		LV2_Feature* f = (LV2_Feature*)malloc(sizeof(LV2_Feature));
		f->URI  = LV2_WORKER__schedule;
		f->data = data;

		return SharedPtr<LV2_Feature>(f, &delete_feature);
	}
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2_REQUEST_RUN_FEATURE_HPP
