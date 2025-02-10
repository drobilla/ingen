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

#include "ClientUpdate.hpp"
#include "CompiledGraph.hpp"
#include "ControlBindings.hpp"
#include "Event.hpp"
#include "SetPortValue.hpp"
#include "State.hpp"
#include "types.hpp"

#include <ingen/Properties.hpp>
#include <ingen/Resource.hpp>
#include <ingen/URI.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ingen {

class Interface;
struct Delta;
struct Put;
struct SetProperty;

namespace server {

class Engine;
class GraphImpl;

namespace events {

/** Set properties of a graph object.
 * \ingroup engine
 */
class Delta : public Event
{
public:
	Delta(Engine&                           engine,
	      const std::shared_ptr<Interface>& client,
	      SampleCount                       timestamp,
	      const ingen::Put&                 msg);

	Delta(Engine&                           engine,
	      const std::shared_ptr<Interface>& client,
	      SampleCount                       timestamp,
	      const ingen::Delta&               msg);

	Delta(Engine&                           engine,
	      const std::shared_ptr<Interface>& client,
	      SampleCount                       timestamp,
	      const ingen::SetProperty&         msg);

	~Delta() override = default;

	void add_set_event(const char* port_symbol,
	                   const void* value,
	                   uint32_t    size,
	                   uint32_t    type);

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& ctx) override;
	void post_process() override;
	void undo(Interface& target) override;

	Execution get_execution() const override;

private:
	enum class Type { SET, PUT, PATCH };

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

	using SetEvents = std::vector<std::unique_ptr<SetPortValue>>;

	void init();

	std::unique_ptr<Event>           _create_event;
	SetEvents                        _set_events;
	std::vector<SpecialType>         _types;
	std::vector<SpecialType>         _remove_types;
	URI                              _subject;
	Properties                       _properties;
	Properties                       _remove;
	ClientUpdate                     _update;
	ingen::Resource*                 _object{nullptr};
	GraphImpl*                       _graph{nullptr};
	std::unique_ptr<CompiledGraph>   _compiled_graph;
	ControlBindings::Binding*        _binding{nullptr};
	StatePtr                         _state;
	Resource::Graph                  _context;
	Type                             _type;

	Properties _added;
	Properties _removed;

	std::vector<ControlBindings::Binding*> _removed_bindings;

	std::optional<Resource> _preset;

	bool _block{false};
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_DELTA_HPP
