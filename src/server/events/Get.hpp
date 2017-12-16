/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include <vector>

#include "BlockFactory.hpp"
#include "ClientUpdate.hpp"
#include "Event.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class BlockImpl;
class GraphImpl;
class PluginImpl;
class PortImpl;

namespace Events {

/** A request from a client to send an object.
 *
 * \ingroup engine
 */
class Get : public Event
{
public:
	Get(Engine&           engine,
	    SPtr<Interface>   client,
	    SampleCount       timestamp,
	    const Ingen::Get& msg);

	bool pre_process(PreProcessContext& ctx);
	void execute(RunContext& context) {}
	void post_process();

private:
	const Ingen::Get      _msg;
	const Node*           _object;
	PluginImpl*           _plugin;
	BlockFactory::Plugins _plugins;
	ClientUpdate          _response;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_GET_HPP
