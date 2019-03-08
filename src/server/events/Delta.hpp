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

#include "CompiledGraph.hpp"
#include "ControlBindings.hpp"
#include "Event.hpp"
#include "PluginImpl.hpp"

namespace ingen {

class Resource;

namespace server {

class Engine;
class GraphImpl;
class RunContext;

namespace events {

class SetPortValue;

/** Set properties of a graph object.
 * \ingroup engine
 */
class Delta : public Event
{
public:
	Delta(Engine&           engine,
	      SPtr<Interface>   client,
	      SampleCount       timestamp,
	      const ingen::Put& msg);

	Delta(Engine&             engine,
	      SPtr<Interface>     client,
	      SampleCount         timestamp,
	      const ingen::Delta& msg);

	Delta(Engine&                   engine,
	      SPtr<Interface>           client,
	      SampleCount               timestamp,
	      const ingen::SetProperty& msg);

	~Delta();

	void add_set_event(const char* port_symbol,
	                   const void* value,
	                   uint32_t    size,
	                   uint32_t    type);

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& context) override;
	void post_process() override;
	void undo(Interface& target) override;

	Execution get_execution() const override;

private:
	enum class Type {
		SET,
		PUT,
		PATCH
	};

	enum class SpecialType {
		NONE,
		ENABLE,
		ENABLE_BROADCAST,
		POLYPHONY,
		POLYPHONIC,
		PORT_INDEX,
		CONTROL_BINDING,
		PRESET,
		LOADED_BUNDLE
	};

	typedef std::vector<UPtr<SetPortValue>> SetEvents;

	void init();

	UPtr<Event>               _create_event;
	SetEvents                 _set_events;
	std::vector<SpecialType>  _types;
	std::vector<SpecialType>  _remove_types;
	URI                       _subject;
	Properties                _properties;
	Properties                _remove;
	ClientUpdate              _update;
	ingen::Resource*          _object;
	GraphImpl*                _graph;
	MPtr<CompiledGraph>       _compiled_graph;
	ControlBindings::Binding* _binding;
	LilvState*                _state;
	Resource::Graph           _context;
	Type                      _type;

	Properties _added;
	Properties _removed;

	std::vector<ControlBindings::Binding*> _removed_bindings;

	boost::optional<Resource> _preset;

	bool _block;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_DELTA_HPP
