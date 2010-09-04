/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#include "raul/Atom.hpp"
#include "module/Module.hpp"
#include "module/World.hpp"
#include "Parser.hpp"
#include "Serialiser.hpp"

using namespace Ingen;

struct IngenModule : public Shared::Module {
	void load(Shared::World* world) {
		world->set_parser(SharedPtr<Serialisation::Parser>(
				new Serialisation::Parser()));
		world->set_serialiser(SharedPtr<Serialisation::Serialiser>(
				new Serialisation::Serialiser(*world, world->store())));
	}
};

static IngenModule* module = NULL;

extern "C" {

Shared::Module*
ingen_module_load() {
	if (!module)
		module = new IngenModule();

	return module;
}

} // extern "C"

