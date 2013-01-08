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

#ifndef INGEN_ENGINE_LV2INFO_HPP
#define INGEN_ENGINE_LV2INFO_HPP

#include "ingen/World.hpp"
#include "lilv/lilv.h"
#include "raul/Noncopyable.hpp"

namespace Ingen {
namespace Server {

/** Stuff that may need to be passed to an LV2 plugin (i.e. LV2 features).
 */
class LV2Info : public Raul::Noncopyable {
public:
	explicit LV2Info(Ingen::World* world);
	~LV2Info();

	LilvNode* const atom_AtomPort;
	LilvNode* const atom_bufferType;
	LilvNode* const atom_supports;
	LilvNode* const lv2_AudioPort;
	LilvNode* const lv2_CVPort;
	LilvNode* const lv2_ControlPort;
	LilvNode* const lv2_InputPort;
	LilvNode* const lv2_OutputPort;
	LilvNode* const lv2_default;
	LilvNode* const lv2_designation;
	LilvNode* const lv2_portProperty;
	LilvNode* const lv2_sampleRate;
	LilvNode* const morph_AutoMorphPort;
	LilvNode* const morph_MorphPort;
	LilvNode* const morph_supportsType;
	LilvNode* const rsz_minimumSize;
	LilvNode* const work_schedule;

	Ingen::World& world()     { return *_world; }
	LilvWorld*    lv2_world() { return _world->lilv_world(); }

private:
	Ingen::World* _world;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_LV2INFO_HPP
