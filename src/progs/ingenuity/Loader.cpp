/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "Loader.h"
#include <fstream>
#include <cassert>
#include <string>
#include "Serializer.h"
#include "PatchModel.h"
using std::cout; using std::endl;

namespace Ingenuity {


Loader::Loader(CountedPtr<ModelEngineInterface> engine)
: _serializer(new Serializer(engine))
{
	assert(_serializer != NULL);
	
	// FIXME: rework this so the thread is only present when it's doing something (save mem)
	start();
}


Loader::~Loader()
{
	delete _serializer;
}


void
Loader::_whipped()
{
	_mutex.lock();
	
	while ( ! _events.empty() ) {
		_events.front()();
		_events.pop_front();
	}

	_mutex.unlock();
}


void
Loader::load_patch(const string&      filename,
	               const string&      parent_path,
	               const string&      name,
	               size_t             poly,
                   const MetadataMap& initial_data,
	               bool               existing)
{
	_mutex.lock();

	_events.push_back(sigc::hide_return(sigc::bind(
		sigc::mem_fun(_serializer, &Serializer::load_patch),
		filename, parent_path, name, poly, initial_data, existing)));
	
	_mutex.unlock();

	whip();
}


void
Loader::save_patch(CountedPtr<PatchModel> model, const string& filename, bool recursive)
{
	_mutex.lock();

	_events.push_back(sigc::hide_return(sigc::bind(
		sigc::mem_fun(this, &Loader::save_patch_event),
		model, filename, recursive)));

	_mutex.unlock();
	
	whip();
}


void
Loader::save_patch_event(CountedPtr<PatchModel> model, const string& filename, bool recursive)
{
	if (recursive)
		cerr << "FIXME: Recursive save." << endl;

	_serializer->start_to_filename(filename);
	_serializer->serialize_patch(model);
	_serializer->finish();
}


} // namespace Ingenuity
