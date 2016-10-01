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

#ifndef INGEN_EVENTS_DELTA_HPP
#define INGEN_EVENTS_DELTA_HPP

#include <vector>

#include <boost/optional.hpp>

#include "lilv/lilv.h"

#include "raul/URI.hpp"

#include "PluginImpl.hpp"
#include "ControlBindings.hpp"
#include "Event.hpp"

namespace Ingen {

class Resource;

namespace Server {

class CompiledGraph;
class Engine;
class GraphImpl;
class RunContext;

namespace Events {

class SetPortValue;

/** Set properties of a graph object.
 * \ingroup engine
 */
class Delta : public Event
{
public:
	enum class Type {
		SET,
		PUT,
		PATCH
	};

	Delta(Engine&                     engine,
	      SPtr<Interface>             client,
	      int32_t                     id,
	      SampleCount                 timestamp,
	      Type                        type,
	      Resource::Graph             context,
	      const Raul::URI&            subject,
	      const Resource::Properties& properties,
	      const Resource::Properties& remove = Resource::Properties());

	~Delta();

	void add_set_event(const char* port_symbol,
	                   const void* value,
	                   uint32_t    size,
	                   uint32_t    type);

	bool pre_process(PreProcessContext& ctx);
	void execute(RunContext& context);
	void post_process();
	void undo(Interface& target);

private:
	enum class SpecialType {
		NONE,
		ENABLE,
		ENABLE_BROADCAST,
		POLYPHONY,
		POLYPHONIC,
		CONTROL_BINDING,
		PRESET,
		LOADED_BUNDLE
	};

	typedef std::vector<SetPortValue*> SetEvents;

	Event*                   _create_event;
	SetEvents                _set_events;
	std::vector<SpecialType> _types;
	std::vector<SpecialType> _remove_types;
	Raul::URI                _subject;
	Resource::Properties     _properties;
	Resource::Properties     _remove;
	ClientUpdate             _update;
	Ingen::Resource*         _object;
	GraphImpl*               _graph;
	CompiledGraph*           _compiled_graph;
	LilvState*               _state;
	Resource::Graph          _context;
	ControlBindings::Key     _binding;
	Type                     _type;

	Resource::Properties _added;
	Resource::Properties _removed;

	SPtr<ControlBindings::Bindings> _old_bindings;

	boost::optional<Resource> _preset;

	std::unique_lock<std::mutex> _poly_lock;  ///< Long-term lock for poly changes
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_DELTA_HPP
