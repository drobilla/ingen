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

#include <fstream>
#include <cassert>
#include <string>
#include "module/global.hpp"
#include "module/World.hpp"
#include "client/PatchModel.hpp"
#include "App.hpp"
#include "ThreadedLoader.hpp"
using std::cout; using std::endl;

namespace Ingen {
namespace GUI {


ThreadedLoader::ThreadedLoader(SharedPtr<EngineInterface> engine)
	: _engine(engine)
	, _deprecated_loader(engine)
{
	set_name("Loader");

	// FIXME: rework this so the thread is only present when it's doing something (save mem)
	if (App::instance().serialisation_module()) {
		Loader* (*new_loader)() = NULL;

		bool found = App::instance().serialisation_module()->get_symbol(
				"new_loader", (void*&)new_loader);

		if (found)
			_loader = SharedPtr<Loader>(new_loader());
	}

	if (_loader) {
		start();
	} else {
		cerr << "WARNING: Failed to load ingen_serialisation module, load disabled." << endl;
		cerr << "(If you are running from the source tree, source set_dev_environment.sh)" << endl;
	}
}


ThreadedLoader::~ThreadedLoader()
{
}


void
ThreadedLoader::_whipped()
{
	_mutex.lock();
	
	while ( ! _events.empty() ) {
		_events.front()();
		_events.pop_front();
	}

	_mutex.unlock();
}

void
ThreadedLoader::load_patch(bool                    merge,
                           const string&           data_base_uri,
                           const Path&             data_path,
                           GraphObject::Variables  engine_data,
                           optional<Path>          engine_parent,
                           optional<const string&> engine_name)
{
	_mutex.lock();

	// FIXME: Filthy hack to load deprecated patches based on file extension
	if (data_base_uri.substr(data_base_uri.length()-3) == ".om") {
		_events.push_back(sigc::hide_return(sigc::bind(
				sigc::mem_fun(_deprecated_loader, &DeprecatedLoader::load_patch),
				data_base_uri,
				engine_parent,
				(engine_name) ? engine_name.get() : "",
				engine_data,
				false)));
	} else {
		_events.push_back(sigc::hide_return(sigc::bind(
				sigc::mem_fun(_loader.get(), &Ingen::Serialisation::Loader::load),
				App::instance().world(),
				data_base_uri,
				engine_parent,
				(engine_name) ? engine_name.get() : "",
				"",
				engine_data )));
	}
	
	whip();
	
	_mutex.unlock();
}


void
ThreadedLoader::save_patch(SharedPtr<PatchModel> model, const string& filename)
{
	_mutex.lock();

	_events.push_back(sigc::hide_return(sigc::bind(
		sigc::mem_fun(this, &ThreadedLoader::save_patch_event),
		model, filename)));

	_mutex.unlock();
	
	whip();
}


void
ThreadedLoader::save_patch_event(SharedPtr<PatchModel> model, const string& filename)
{
	if (App::instance().serialiser())
		App::instance().serialiser()->to_file(model, filename);
}


} // namespace GUI
} // namespace Ingen
