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

#include "raul/log.hpp"

#include "ingen/Module.hpp"
#include "ingen/World.hpp"

#include "../server/Engine.hpp"
#include "../server/EventWriter.hpp"

#include "SocketListener.hpp"

using namespace Ingen;

struct IngenSocketServerModule : public Ingen::Module {
	void load(Ingen::World* world) {
		listener = SharedPtr<Ingen::Socket::SocketListener>(
			new Ingen::Socket::SocketListener(*world));
	}

	SharedPtr<Ingen::Socket::SocketListener> listener;
};

extern "C" {

Ingen::Module*
ingen_module_load()
{
	return new IngenSocketServerModule();
}

} // extern "C"
