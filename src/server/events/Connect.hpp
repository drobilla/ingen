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

#ifndef INGEN_EVENTS_CONNECT_HPP
#define INGEN_EVENTS_CONNECT_HPP

#include "raul/Path.hpp"

#include "Event.hpp"
#include "PortImpl.hpp"
#include "types.hpp"

namespace Raul {
template <typename T> class Array;
}

namespace Ingen {
namespace Server {

class ArcImpl;
class CompiledGraph;
class GraphImpl;
class InputPort;

namespace Events {

/** Make an Arc between two Ports.
 *
 * \ingroup engine
 */
class Connect : public Event
{
public:
	Connect(Engine&           engine,
	        SPtr<Interface>   client,
	        int32_t           id,
	        SampleCount       timestamp,
	        const Raul::Path& tail,
	        const Raul::Path& head);

	bool pre_process(PreProcessContext& ctx);
	void execute(RunContext& context);
	void post_process();
	void undo(Interface& target);

private:
	const Raul::Path       _tail_path;
	const Raul::Path       _head_path;
	GraphImpl*             _graph;
	InputPort*             _head;
	MPtr<CompiledGraph>    _compiled_graph;
	SPtr<ArcImpl>          _arc;
	MPtr<PortImpl::Voices> _voices;
	Properties             _tail_remove;
	Properties             _tail_add;
	Properties             _head_remove;
	Properties             _head_add;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_CONNECT_HPP
