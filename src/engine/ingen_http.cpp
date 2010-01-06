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

#include "module/Module.hpp"
#include "module/World.hpp"
#include "HTTPEngineReceiver.hpp"
#include "Engine.hpp"
#include "tuning.hpp"

using namespace std;
using namespace Ingen;

struct IngenHTTPModule : public Ingen::Shared::Module {
	void load(Ingen::Shared::World* world) {
		SharedPtr<HTTPEngineReceiver> interface(
				new Ingen::HTTPEngineReceiver(*world->local_engine.get(),
						world->conf->option("engine-port").get_int32()));
		world->local_engine->add_event_source(interface);
	}
};

static IngenHTTPModule* module = NULL;

extern "C" {

Ingen::Shared::Module*
ingen_module_load() {
	if (!module)
		module = new IngenHTTPModule();

	return module;
}

} // extern "C"
