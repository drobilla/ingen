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

#ifndef INGEN_GUI_THREADEDLOADER_HPP
#define INGEN_GUI_THREADEDLOADER_HPP

#include <string>
#include <list>
#include <cassert>
#include <boost/optional/optional.hpp>
#include "raul/Thread.hpp"
#include "raul/Slave.hpp"
#include <glibmm/thread.h>
#include "ingen/Interface.hpp"
#include "ingen/serialisation/Serialiser.hpp"
#include "ingen/serialisation/Parser.hpp"

using std::string;
using std::list;
using boost::optional;

namespace Ingen {
using namespace Shared;
using namespace Client;
using namespace Serialisation;

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
	ThreadedLoader(App&                 app,
	               SharedPtr<Interface> engine);

	void load_patch(bool                              merge,
                    const Glib::ustring&              document_uri,
                    optional<Raul::Path>              engine_parent,
                    optional<Raul::Symbol>            engine_symbol,
                    optional<GraphObject::Properties> engine_data);

	void save_patch(SharedPtr<const PatchModel> model, const string& filename);

	SharedPtr<Parser> parser();

private:
	void save_patch_event(SharedPtr<const PatchModel> model, const string& filename);

	/** Returns nothing and takes no parameters (because they have all been bound) */
	typedef sigc::slot<void> Closure;

	void _whipped();

	App&                 _app;
	SharedPtr<Interface> _engine;
	Glib::Mutex          _mutex;
	list<Closure>        _events;
};

} // namespace GUI
} // namespace Ingen

#endif // INGEN_GUI_LOADERRTHREAD_HPP
