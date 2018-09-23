/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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

#include "CompiledGraph.hpp"
#include "Disconnect.hpp"
#include "Event.hpp"

namespace ingen {
namespace server {

class BlockImpl;
class GraphImpl;
class PortImpl;

namespace events {

class Disconnect;

/** An event to disconnect all connections to a Block.
 *
 * \ingroup engine
 */
class DisconnectAll : public Event
{
public:
	DisconnectAll(Engine&                     engine,
	              SPtr<Interface>             client,
	              SampleCount                 timestamp,
	              const ingen::DisconnectAll& msg);

	DisconnectAll(Engine&    engine,
	              GraphImpl* parent,
	              Node*      object);

	~DisconnectAll();

	bool pre_process(PreProcessContext& ctx);
	void execute(RunContext& context);
	void post_process();
	void undo(Interface& target);

private:
	typedef std::list<Disconnect::Impl*> Impls;

	const ingen::DisconnectAll _msg;
	GraphImpl*                 _parent;
	BlockImpl*                 _block;
	PortImpl*                  _port;
	Impls                      _impls;
	MPtr<CompiledGraph>        _compiled_graph;
	bool                       _deleting;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_DISCONNECTALL_HPP
