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

#include "ThreadedLoader.h"
#include <fstream>
#include <cassert>
#include <string>
#include "PatchModel.h"
using std::cout; using std::endl;

namespace Ingenuity {


ThreadedLoader::ThreadedLoader(SharedPtr<ModelEngineInterface> engine)
	: _deprecated_loader(engine)
	, _loader(engine)
{
	// FIXME: rework this so the thread is only present when it's doing something (save mem)
	start();
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

/** FIXME: use poly parameter */
void
ThreadedLoader::load_patch(bool                    merge,
                           const string&           data_base_uri,
                           const Path&             data_path,
                           MetadataMap             engine_data,
                           const Path&             engine_parent,
                           optional<const string&> engine_name,
                           optional<size_t>        engine_poly)
{
	_mutex.lock();

	// FIXME: Filthy hack to load deprecated patches based on file extension
	if (data_base_uri.substr(data_base_uri.length()-3) == ".om") {
		_events.push_back(sigc::hide_return(sigc::bind(
				sigc::mem_fun(_deprecated_loader, &DeprecatedLoader::load_patch),
				data_base_uri,
				engine_parent,
				(engine_name) ? engine_name.get() : "",
				(engine_poly) ? engine_poly.get() : 1,
				engine_data,
				false)));
	} else {
		_events.push_back(sigc::hide_return(sigc::bind(
				sigc::mem_fun(_loader, &Loader::load),
				data_base_uri,
				engine_parent,
				(engine_name) ? engine_name.get() : "",
				// FIXME: poly here
				"",
				engine_data )));
	}
	
	_mutex.unlock();

	whip();
}


void
ThreadedLoader::save_patch(SharedPtr<PatchModel> model, const string& filename, bool recursive)
{
	_mutex.lock();

	_events.push_back(sigc::hide_return(sigc::bind(
		sigc::mem_fun(this, &ThreadedLoader::save_patch_event),
		model, filename, recursive)));

	_mutex.unlock();
	
	whip();
}


void
ThreadedLoader::save_patch_event(SharedPtr<PatchModel> model, const string& filename, bool recursive)
{
	if (recursive)
		cerr << "FIXME: Recursive save." << endl;

	_serializer.start_to_filename(filename);
	_serializer.serialize(model);
	_serializer.finish();
}


} // namespace Ingenuity
