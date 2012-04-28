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

#define __STDC_LIMIT_MACROS 1

#include <assert.h>
#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

#include "ingen/shared/World.hpp"
#include "ingen/shared/LV2Features.hpp"

#include "LV2Info.hpp"
#include "LV2RequestRunFeature.hpp"
#include "LV2ResizeFeature.hpp"

namespace Ingen {
namespace Server {

LV2Info::LV2Info(Ingen::Shared::World* world)
	: input_class(lilv_new_uri(world->lilv_world(), LV2_CORE__InputPort))
	, output_class(lilv_new_uri(world->lilv_world(), LV2_CORE__OutputPort))
	, control_class(lilv_new_uri(world->lilv_world(), LV2_CORE__ControlPort))
	, cv_class(lilv_new_uri(world->lilv_world(), LV2_CORE__CVPort))
	, audio_class(lilv_new_uri(world->lilv_world(), LV2_CORE__AudioPort))
	, atom_port_class(lilv_new_uri(world->lilv_world(), LV2_ATOM__AtomPort))
	, _world(world)
{
	assert(world);

	world->lv2_features()->add_feature(
			SharedPtr<Shared::LV2Features::Feature>(new ResizeFeature()));
	world->lv2_features()->add_feature(
			SharedPtr<Shared::LV2Features::Feature>(new RequestRunFeature()));
}

LV2Info::~LV2Info()
{
	lilv_node_free(input_class);
	lilv_node_free(output_class);
	lilv_node_free(control_class);
	lilv_node_free(cv_class);
	lilv_node_free(audio_class);
	lilv_node_free(atom_port_class);
}

} // namespace Server
} // namespace Ingen
