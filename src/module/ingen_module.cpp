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

#include "redlandmm/World.hpp"
#include "uri-map.lv2/uri-map.h"
#include "ingen-config.h"
#include "shared/LV2Features.hpp"
#include "shared/LV2URIMap.hpp"
#include "ingen_module.hpp"
#include "World.hpp"
#ifdef HAVE_SLV2
#include "slv2/slv2.h"
#endif

extern "C" {

static Ingen::Shared::World* world = NULL;

Ingen::Shared::World*
ingen_get_world()
{
    static Ingen::Shared::World* world = NULL;

	if (!world) {
		world = new Ingen::Shared::World();
        world->rdf_world = new Redland::World();
#ifdef HAVE_SLV2
		world->slv2_world = slv2_world_new_using_rdf_world(world->rdf_world->world());
		world->lv2_features = new Ingen::Shared::LV2Features();
		world->uris = PtrCast<Ingen::Shared::LV2URIMap>(
				world->lv2_features->feature(LV2_URI_MAP_URI));
		slv2_world_load_all(world->slv2_world);
#else
		world->uris = SharedPtr<Ingen::Shared::LV2URIMap>(
				new Ingen::Shared::LV2URIMap());
#endif
		world->engine.reset();
		world->local_engine.reset();
	}

	return world;
}

void
ingen_destroy_world()
{
	if (world) {
		world->unload_all();
#ifdef HAVE_SLV2
		slv2_world_free(world->slv2_world);
		delete world->lv2_features;
#endif
        delete world->rdf_world;
		delete world;
		world = NULL;
	}
}

} // extern "C"
