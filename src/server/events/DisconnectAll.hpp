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

#ifndef INGEN_EVENTS_DISCONNECTALL_HPP
#define INGEN_EVENTS_DISCONNECTALL_HPP

#include <list>

#include "raul/Path.hpp"

#include "Disconnect.hpp"
#include "Event.hpp"

namespace Ingen {
namespace Server {

class CompiledPatch;
class NodeImpl;
class PatchImpl;
class PortImpl;

namespace Events {

class Disconnect;

/** An event to disconnect all connections to a Node.
 *
 * \ingroup engine
 */
class DisconnectAll : public Event
{
public:
	DisconnectAll(Engine&              engine,
	              SharedPtr<Interface> client,
	              int32_t              id,
	              SampleCount          timestamp,
	              const Raul::Path&    parent,
	              const Raul::Path&    object);

	DisconnectAll(Engine&          engine,
	              PatchImpl*       parent,
	              GraphObjectImpl* object);

	~DisconnectAll();

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::Path _parent_path;
	Raul::Path _path;
	PatchImpl* _parent;
	NodeImpl*  _node;
	PortImpl*  _port;

	typedef std::list<Disconnect::Impl*> Impls;
	Impls _impls;

	CompiledPatch* _compiled_patch; ///< New process order for Patch

	bool _deleting;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_DISCONNECTALL_HPP
