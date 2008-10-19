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

#include <string>
#include <sstream>
#include <iostream>
#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "raul/SharedPtr.hpp"

#ifndef INGEN_MODULE_DIR
#error This file expects INGEN_MODULE_DIR to be defined.
#endif

using namespace std;

namespace Ingen {
namespace Shared {


/** Load a dynamic module from the default path.
 *
 * This will check in the directories specified in the environment variable
 * INGEN_MODULE_PATH (typical colon delimited format), then the default module
 * installation directory (ie /usr/local/lib/ingen), in that order.
 *
 * \param name The base name of the module, e.g. "ingen_serialisation"
 */
SharedPtr<Glib::Module>
load_module(const string& name)
{
	Glib::Module* module = NULL;

	// Search INGEN_MODULE_PATH first
	bool module_path_found;
	string module_path = Glib::getenv("INGEN_MODULE_PATH", module_path_found);
	if (module_path_found) {
		string dir;
		istringstream iss(module_path);
		while (getline(iss, dir, ':')) {
			string filename = Glib::Module::build_path(dir, name);
			if (Glib::file_test(filename, Glib::FILE_TEST_EXISTS)) {
				module = new Glib::Module(filename, Glib::MODULE_BIND_LAZY);
				if (*module) {
					cerr << "Loaded module \"" <<  name << "\" from " << filename << endl;
					return SharedPtr<Glib::Module>(module);
				} else {
					delete module;
					cerr << Glib::Module::get_last_error() << endl;
				}
			}
		}
	}

	// Try default directory if not found
	module = new Glib::Module(
			Glib::Module::build_path(INGEN_MODULE_DIR, name),
            Glib::MODULE_BIND_LAZY);

	if (*module) {
		cerr << "Loaded module \"" <<  name << "\" from " << INGEN_MODULE_DIR << endl;
		return SharedPtr<Glib::Module>(module);
	} else if (!module_path_found) {
		cerr << "Unable to find module " << name
			<< " (" << Glib::Module::get_last_error() << ")" << endl;
		return SharedPtr<Glib::Module>();
	} else {
		cerr << "Unable to load module " << name << " from " << module_path
			<< " (" << Glib::Module::get_last_error() << ")" << endl;
		cerr << "Is Ingen installed?  Use ./ingen.dev to run from the source tree." << endl;
		return SharedPtr<Glib::Module>();
	}
}


} // namespace Shared
} // namespace Ingen

