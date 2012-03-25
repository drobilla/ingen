/* This file is part of Ingen.
 * Copyright 2008-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_LV2INFO_HPP
#define INGEN_ENGINE_LV2INFO_HPP

#include <map>
#include <string>

#include "lilv/lilv.h"
#include "ingen/shared/World.hpp"

namespace Ingen {

class Node;

namespace Server {

/** Stuff that may need to be passed to an LV2 plugin (i.e. LV2 features).
 */
class LV2Info {
public:
	explicit LV2Info(Ingen::Shared::World* world);
	~LV2Info();

	LilvNode* input_class;
	LilvNode* output_class;
	LilvNode* control_class;
	LilvNode* cv_class;
	LilvNode* audio_class;
	LilvNode* atom_port_class;

	Ingen::Shared::World& world()     { return *_world; }
	LilvWorld*            lv2_world() { return _world->lilv_world(); }

private:
	Ingen::Shared::World* _world;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2INFO_HPP
