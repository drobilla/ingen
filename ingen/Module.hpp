/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_MODULE_HPP
#define INGEN_MODULE_HPP

#include <memory>

#include "ingen/FilePath.hpp"
#include "ingen/Library.hpp"
#include "ingen/ingen.h"

namespace ingen {

class World;

/** A dynamically loaded Ingen module.
 *
 * All components of Ingen reside in one of these.
 * @ingroup IngenShared
 */
class INGEN_API Module {
public:
	Module() : library(nullptr) {}
	virtual ~Module() = default;

	Module(const Module&) = delete;
	Module& operator=(const Module&) = delete;

	virtual void load(ingen::World* world) = 0;
	virtual void run(ingen::World* world) {}

	/** Library implementing this module.
	 *
	 * This is managed by the World and not this class, since closing the library
	 * in this destructor could possibly reference code from the library
	 * afterwards and cause a segfault on exit.
	 */
	std::unique_ptr<Library> library;
};

} // namespace ingen

extern "C" {

/** Prototype for the ingen_module_load() entry point in an ingen module. */
INGEN_API ingen::Module* ingen_module_load();

}

#endif // INGEN_MODULE_HPP
