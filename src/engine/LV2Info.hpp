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

#ifndef INGEN_ENGINE_LV2INFO_HPP
#define INGEN_ENGINE_LV2INFO_HPP

#include "ingen-config.h"
#ifndef HAVE_SLV2
#error "This file requires SLV2, but HAVE_SLV2 is not defined.  Please report."
#endif

#include <map>
#include <string>
#include "slv2/slv2.h"
#include "module/World.hpp"
#include "shared/LV2URIMap.hpp"
#include "shared/LV2Features.hpp"
#include "uri-map.lv2/uri-map.h"
#include "event.lv2/event.h"
#include "object.lv2/object.h"
#include "resize-port.lv2/resize-port.h"

namespace Ingen {

namespace Shared { class Node; }

/** Stuff that may need to be passed to an LV2 plugin (i.e. LV2 features).
 */
class LV2Info {
public:
	LV2Info(Ingen::Shared::World* world);
	~LV2Info();

	SLV2Value input_class;
	SLV2Value output_class;
	SLV2Value control_class;
	SLV2Value audio_class;
	SLV2Value event_class;
	SLV2Value value_port_class;
	SLV2Value message_port_class;

	Ingen::Shared::World& world()     { return *_world; }
	SLV2World             lv2_world() { return _world->slv2_world; }

private:
	Ingen::Shared::World* _world;
};


} // namespace Ingen

#endif // INGEN_ENGINE_LV2INFO_HPP
