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
#include "PatchLibrarian.h"
#include "PatchModel.h"
using std::cout; using std::endl;

namespace Ingenuity {


Loader::Loader(CountedPtr<ModelEngineInterface> engine)
: _patch_librarian(new PatchLibrarian(engine))
{
	assert(_patch_librarian != NULL);
	
	// FIXME: rework this so the thread is only present when it's doing something (save mem)
	start();
}


Loader::~Loader()
{
	delete _patch_librarian;
}


void
Loader::_whipped()
{
	_mutex.lock();

	Closure& ev = _event;
	ev();
	ev.disconnect();

	_cond.signal();
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

	_event = sigc::hide_return(sigc::bind(
		sigc::mem_fun(_patch_librarian, &PatchLibrarian::load_patch),
		filename, parent_path, name, poly, initial_data, existing));
	
	whip();

	_cond.wait(_mutex);
	_mutex.unlock();
}


void
Loader::save_patch(CountedPtr<PatchModel> model, const string& filename, bool recursive)
{
	cerr << "FIXME: (loader) save patch\n";
	//cout << "[Loader] Saving patch " << filename << endl;
	//set_event(new SavePatchEvent(m_patch_librarian, model, filename, recursive));
}

} // namespace Ingenuity
