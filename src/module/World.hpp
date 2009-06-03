/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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

#include <boost/shared_ptr.hpp>
#include <glibmm/module.h>

typedef struct _SLV2World* SLV2World;

namespace Redland { class World; }

namespace Ingen {

class Engine;

namespace Serialisation { class Serialiser; class Parser; }

namespace Shared {

class EngineInterface;
class Store;
class LV2Features;


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
	Redland::World* rdf_world;

    SLV2World    slv2_world;
	LV2Features* lv2_features;

    boost::shared_ptr<EngineInterface>           engine;
    boost::shared_ptr<Engine>                    local_engine;
    boost::shared_ptr<Serialisation::Serialiser> serialiser;
    boost::shared_ptr<Serialisation::Parser>     parser;
    boost::shared_ptr<Store>                     store;

	boost::shared_ptr<Glib::Module> serialisation_module;
};


} // namespace Shared
} // namespace Ingen

#endif // INGEN_WORLD_HPP

