/* This file is part of Ingen.
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#define __STDC_LIMIT_MACROS 1
#include <cassert>
#include <iostream>
#include <stdint.h>
#include "object.lv2/object.h"
#include "LV2Info.hpp"
#include "module/World.hpp"
#include "LV2Features.hpp"
#include "LV2EventFeature.hpp"
#include "LV2BlobFeature.hpp"
#include "LV2ResizeFeature.hpp"

using namespace std;

namespace Ingen {

LV2Info::LV2Info(Ingen::Shared::World* world)
	: input_class(slv2_value_new_uri(world->slv2_world, SLV2_PORT_CLASS_INPUT))
	, output_class(slv2_value_new_uri(world->slv2_world, SLV2_PORT_CLASS_OUTPUT))
	, control_class(slv2_value_new_uri(world->slv2_world, SLV2_PORT_CLASS_CONTROL))
	, audio_class(slv2_value_new_uri(world->slv2_world, SLV2_PORT_CLASS_AUDIO))
	, event_class(slv2_value_new_uri(world->slv2_world, SLV2_PORT_CLASS_EVENT))
	, object_port_class(slv2_value_new_uri(world->slv2_world, LV2_OBJECT_URI "#ObjectPort"))
	, _world(world)
{
	assert(world);

	world->lv2_features->add_feature(LV2_EVENT_URI,
			SharedPtr<Shared::LV2Features::Feature>(new EventFeature()));
	world->lv2_features->add_feature(LV2_BLOB_SUPPORT_URI,
			SharedPtr<Shared::LV2Features::Feature>(new BlobFeature()));
	world->lv2_features->add_feature(LV2_RESIZE_PORT_URI,
			SharedPtr<Shared::LV2Features::Feature>(new ResizeFeature()));
}

LV2Info::~LV2Info()
{
	slv2_value_free(input_class);
	slv2_value_free(output_class);
	slv2_value_free(control_class);
	slv2_value_free(audio_class);
	slv2_value_free(event_class);
	slv2_value_free(object_port_class);
}

} // namespace Ingen
