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

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"

#include "ingen/shared/World.hpp"
#include "ingen/shared/LV2Features.hpp"

#include "LV2Info.hpp"
#include "LV2ResizeFeature.hpp"

namespace Ingen {
namespace Server {

LV2Info::LV2Info(Ingen::Shared::World* world)
	: atom_AtomPort(lilv_new_uri(world->lilv_world(), LV2_ATOM__AtomPort))
	, atom_bufferType(lilv_new_uri(world->lilv_world(), LV2_ATOM__bufferType))
	, atom_supports(lilv_new_uri(world->lilv_world(), LV2_ATOM__supports))
	, lv2_AudioPort(lilv_new_uri(world->lilv_world(), LV2_CORE__AudioPort))
	, lv2_CVPort(lilv_new_uri(world->lilv_world(), LV2_CORE__CVPort))
	, lv2_ControlPort(lilv_new_uri(world->lilv_world(), LV2_CORE__ControlPort))
	, lv2_InputPort(lilv_new_uri(world->lilv_world(), LV2_CORE__InputPort))
	, lv2_OutputPort(lilv_new_uri(world->lilv_world(), LV2_CORE__OutputPort))
	, lv2_default(lilv_new_uri(world->lilv_world(), LV2_CORE__default))
	, lv2_portProperty(lilv_new_uri(world->lilv_world(), LV2_CORE__portProperty))
	, morph_AutoMorphPort(lilv_new_uri(world->lilv_world(), LV2_MORPH__AutoMorphPort))
	, morph_MorphPort(lilv_new_uri(world->lilv_world(), LV2_MORPH__MorphPort))
	, morph_supportsType(lilv_new_uri(world->lilv_world(), LV2_MORPH__supportsType))
	, rsz_minimumSize(lilv_new_uri(world->lilv_world(), LV2_RESIZE_PORT__minimumSize))
	, work_schedule(lilv_new_uri(world->lilv_world(), LV2_WORKER__schedule))
	, _world(world)
{
	assert(world);

	world->lv2_features().add_feature(
		SharedPtr<Shared::LV2Features::Feature>(new ResizeFeature()));
}

LV2Info::~LV2Info()
{
	lilv_node_free(rsz_minimumSize);
	lilv_node_free(lv2_portProperty);
	lilv_node_free(lv2_default);
	lilv_node_free(lv2_OutputPort);
	lilv_node_free(lv2_InputPort);
	lilv_node_free(lv2_ControlPort);
	lilv_node_free(lv2_CVPort);
	lilv_node_free(lv2_AudioPort);
	lilv_node_free(atom_supports);
	lilv_node_free(atom_bufferType);
	lilv_node_free(atom_AtomPort);
}

} // namespace Server
} // namespace Ingen
