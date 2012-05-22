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

#ifndef INGEN_EVENTS_CREATENODE_HPP
#define INGEN_EVENTS_CREATENODE_HPP

#include <list>
#include <utility>

#include "ingen/Resource.hpp"

#include "Event.hpp"

namespace Ingen {
namespace Server {

class PatchImpl;
class NodeImpl;
class CompiledPatch;

namespace Events {

/** An event to load a Node and insert it into a Patch.
 *
 * \ingroup engine
 */
class CreateNode : public Event
{
public:
	CreateNode(Engine&                     engine,
	           Interface*                  client,
	           int32_t                     id,
	           SampleCount                 timestamp,
	           const Raul::Path&           node_path,
	           const Resource::Properties& properties);

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	/// Update put message to broadcast to clients
	typedef std::list< std::pair<Raul::URI, Resource::Properties> > Update;

	Raul::Path           _path;
	std::string          _plugin_uri;
	Resource::Properties _properties;
	Update               _update;
	PatchImpl*           _patch;
	NodeImpl*            _node;
	CompiledPatch*       _compiled_patch; ///< Patch's new process order
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_CREATENODE_HPP
