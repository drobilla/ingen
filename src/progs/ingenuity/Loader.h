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

#ifndef LOADERTHREAD_H
#define LOADERTHREAD_H

#include <string>
#include <list>
#include <cassert>
#include "util/Thread.h"
#include "util/Slave.h"
#include "util/Mutex.h"
#include "util/Condition.h"
#include "ModelEngineInterface.h"
#include "ObjectModel.h"
using std::string;
using std::list;

namespace Ingen { namespace Client {
	class Serializer;
	class PatchModel;
} }
using namespace Ingen::Client;

namespace Ingenuity {


/** Thread for loading patch files.
 *
 * This is a seperate thread so it can send all the loading message without
 * blocking everything else, so the app can respond to the incoming events
 * caused as a result of the patch loading, while the patch loads.
 *
 * Implemented as a slave with a list of closures (events) which processes
 * all events in the (mutex protected) list each time it's whipped.
 *
 * \ingroup Ingenuity
 */
class Loader : public Slave
{
public:
	Loader(CountedPtr<ModelEngineInterface> engine);
	~Loader();

	Serializer& serializer() const { return *_serializer; }

	void load_patch(const string&      filename,
	                const string&      parent_path,
	                const string&      name,
	                size_t             poly,
	                const MetadataMap& initial_data,
	                bool               merge = false);
	
	void save_patch(CountedPtr<PatchModel> model, const string& filename, bool recursive);


private:	

	void save_patch_event(CountedPtr<PatchModel> model, const string& filename, bool recursive);
	
	/** Returns nothing and takes no parameters (because they have all been bound) */
	typedef sigc::slot<void> Closure;

	void _whipped();

	Serializer* const _serializer;
	Mutex             _mutex;
	list<Closure>     _events;
};


} // namespace Ingenuity

#endif // LOADERRTHREAD_H
