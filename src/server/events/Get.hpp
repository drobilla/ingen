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


#ifndef INGEN_EVENTS_GET_HPP
#define INGEN_EVENTS_GET_HPP

#include <glibmm/thread.h>

#include "Event.hpp"
#include "NodeFactory.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class GraphObjectImpl;
class PluginImpl;

namespace Events {

/** A request from a client to send an object.
 *
 * \ingroup engine
 */
class Get : public Event
{
public:
	Get(Engine&          engine,
	    Interface*       client,
	    int32_t          id,
	    SampleCount      timestamp,
	    const Raul::URI& uri);

	void pre_process();
	void post_process();

private:
	const Raul::URI          _uri;
	const GraphObjectImpl*   _object;
	const PluginImpl*        _plugin;
	NodeFactory::Plugins     _plugins;
	Glib::RWLock::ReaderLock _lock;
};

} // namespace Server
} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_GET_HPP
