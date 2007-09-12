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

#ifndef THREADEDLOADER_H
#define THREADEDLOADER_H

#include <string>
#include <list>
#include <cassert>
#include <boost/optional/optional.hpp>
#include <raul/Thread.hpp>
#include <raul/Slave.hpp>
#include <glibmm/thread.h>
#include "interface/EngineInterface.hpp"
#include "client/PatchModel.hpp"
#include "client/Serializer.hpp"
#include "client/DeprecatedLoader.hpp"
#include "serialisation/Loader.hpp"
using std::string;
using std::list;
using boost::optional;

using namespace Ingen::Client;
using namespace Ingen::Serialisation;

namespace Ingen {
namespace GUI {


/** Thread for loading patch files.
 *
 * This is a seperate thread so it can send all the loading message without
 * blocking everything else, so the app can respond to the incoming events
 * caused as a result of the patch loading, while the patch loads.
 *
 * Implemented as a slave with a list of closures (events) which processes
 * all events in the (mutex protected) list each time it's whipped.
 *
 * \ingroup GUI
 */
class ThreadedLoader : public Raul::Slave
{
public:
	ThreadedLoader(SharedPtr<EngineInterface> engine);
	~ThreadedLoader();

	// FIXME: there's a pattern here....
	// (same core interface as Loader/Serializer)
	
	void load_patch(bool                    merge,
	                const string&           data_base_uri,
	                const Path&             data_path,
	                MetadataMap             engine_data,
	                optional<Path>          engine_parent,
	                optional<const string&> engine_name = optional<const string&>());
	
	void save_patch(SharedPtr<PatchModel> model, const string& filename);

private:	

	void save_patch_event(SharedPtr<PatchModel> model, const string& filename);
	
	/** Returns nothing and takes no parameters (because they have all been bound) */
	typedef sigc::slot<void> Closure;

	void _whipped();

	SharedPtr<Glib::Module> _serialisation_module;

	SharedPtr<EngineInterface> _engine;
	SharedPtr<Loader>          _loader;

	DeprecatedLoader _deprecated_loader;
	Serializer       _serializer;
	Glib::Mutex      _mutex;
	list<Closure>    _events;
};


} // namespace GUI
} // namespace Ingen

#endif // LOADERRTHREAD_H
