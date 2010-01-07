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

#include <glibmm/module.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include "raul/log.hpp"
#include "ingen-config.h"
#include "shared/runtime_paths.hpp"
#include "World.hpp"

#define LOG(s) s << "[Module] "

using namespace std;
using namespace Raul;

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
static SharedPtr<Glib::Module>
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
					LOG(info) << "Loaded `" <<  name << "' from " << filename << endl;
					return SharedPtr<Glib::Module>(module);
				} else {
					delete module;
					error << Glib::Module::get_last_error() << endl;
				}
			}
		}
	}

	// Try default directory if not found
	module = new Glib::Module(
			Shared::module_path(name),
            Glib::MODULE_BIND_LAZY);

	if (*module) {
		LOG(info) << "Loaded `" <<  name << "' from " << INGEN_MODULE_DIR << endl;
		return SharedPtr<Glib::Module>(module);
	} else if (!module_path_found) {
		LOG(error) << "Unable to find " << name
			<< " (" << Glib::Module::get_last_error() << ")" << endl;
		return SharedPtr<Glib::Module>();
	} else {
		LOG(error) << "Unable to load " << name << " from " << module_path
			<< " (" << Glib::Module::get_last_error() << ")" << endl;
		LOG(error) << "Is Ingen installed?" << endl;
		return SharedPtr<Glib::Module>();
	}
}

/** Load an Ingen module.
 * @return true on success, false on failure
 */
bool
World::load(const char* name)
{
	SharedPtr<Glib::Module> lib = load_module(name);
	Ingen::Shared::Module* (*module_load)() = NULL;
	if (lib && lib->get_symbol("ingen_module_load", (void*&)module_load)) {
		Module* module = module_load();
		module->library = lib;
		module->load(this);
		modules.insert(make_pair(string(name), module));
		return true;
	} else {
		LOG(error) << "Failed to load module " << name << endl;
		return false;
	}
}


/** Unload all loaded Ingen modules.
 */
void
World::unload_all()
{
	modules.clear();
}


/** Get an interface for a remote engine at @a url
 */
SharedPtr<Ingen::Shared::EngineInterface>
World::interface(const std::string& url)
{
	const string scheme = url.substr(0, url.find(":"));
	const InterfaceFactories::const_iterator i = interface_factories.find(scheme);
	if (i == interface_factories.end()) {
		warn << "Unknown URI scheme `'" << scheme << "'" << endl;
		return SharedPtr<Ingen::Shared::EngineInterface>();
	}

	return i->second(this, url);
}


/** Run a script of type @a mime_type at filename @a filename */
bool
World::run(const std::string& mime_type, const std::string& filename)
{
	const ScriptRunners::const_iterator i = script_runners.find(mime_type);
	if (i == script_runners.end()) {
		warn << "Unknown script MIME type `'" << mime_type << "'" << endl;
		return false;
	}

	return i->second(this, filename.c_str());
}

void
World::add_interface_factory(const std::string& scheme, InterfaceFactory factory)
{
	interface_factories.insert(make_pair(scheme, factory));
}



} // namespace Shared
} // namespace Ingen
