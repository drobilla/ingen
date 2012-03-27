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


#ifndef INGEN_SHARED_MODULE_HPP
#define INGEN_SHARED_MODULE_HPP

#include <glibmm/module.h>

#include "raul/SharedPtr.hpp"

namespace Ingen {
namespace Shared {

class World;

/** A dynamically loaded Ingen module.
 *
 * All components of Ingen reside in one of these.
 */
struct Module {
	virtual ~Module();

	virtual void load(Ingen::Shared::World* world) = 0;
	virtual void run(Ingen::Shared::World* world) {}

	SharedPtr<Glib::Module> library;
};

} // namespace Shared
} // namespace Ingen

#endif // INGEN_SHARED_MODULE_HPP
