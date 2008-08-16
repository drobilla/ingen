/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef INGEN_WORLD_HPP
#define INGEN_WORLD_HPP

#include CONFIG_H_PATH

#include <string>
#include <glibmm/module.h>
#include <raul/SharedPtr.hpp>
#include "shared/LV2Features.hpp"

#ifdef HAVE_SLV2
#include <slv2/slv2.h>
#endif

namespace Redland { class World; }

namespace Ingen {
class Engine;

namespace Serialisation { class Serialiser; class Loader; }
using Serialisation::Serialiser;
using Serialisation::Loader;

namespace Shared {
class EngineInterface;
class Store;


/** The "world" all Ingen modules may share.
 *
 * This is required for shared access to things like Redland, so locking can
 * take place centrally and the engine/gui using the same library won't
 * explode horribly.
 *
 * Hopefully at some point in the future it can allow some fun things like
 * scripting bindings that play with all loaded components of
 * The Ingen System(TM) and whatnot.
 */
struct World {
#ifdef HAVE_SLV2
	SLV2World    slv2_world;
	LV2Features* lv2_features;
#endif

	Redland::World* rdf_world;

    SharedPtr<EngineInterface> engine;
    SharedPtr<Engine>          local_engine;
    SharedPtr<Serialiser>      serialiser;
    SharedPtr<Loader>          loader;
    SharedPtr<Store>           store;
	
	SharedPtr<Glib::Module> serialisation_module;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_WORLD_HPP

