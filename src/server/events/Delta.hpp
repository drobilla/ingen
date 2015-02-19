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

#ifndef INGEN_EVENTS_SETMETADATA_HPP
#define INGEN_EVENTS_SETMETADATA_HPP

#include <vector>

#include "lilv/lilv.h"

#include "raul/URI.hpp"

#include "ControlBindings.hpp"
#include "Event.hpp"

namespace Ingen {

class Resource;

namespace Server {

class CompiledGraph;
class Engine;
class GraphImpl;
class ProcessContext;

namespace Events {

/** \page methods
 * <h2>POST</h2>
 * As per HTTP (RFC2616 S9.5).
 *
 * Append properties to a graph object.
 *
 * An object can have several properties with a single predicate.
 * POST appends properties without modifying or removing existing properties.
 */

/** \page methods
 * <h2>PUT</h2>
 * As per HTTP (RFC2616 S9.6).
 *
 * Set properties of a graph object, or create an object.
 *
 * An object can have several properties with a single predicate.
 * \li If the object does not yet exist, the message must contain sufficient
 * information to create the object (e.g. known rdf:type properties, etc.)
 * \li If the object does exist, a PUT removes all existing object properties
 * with predicates that match any property in the message, then adds all
 * properties from the message.
 */

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

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum class SpecialType {
		NONE,
		ENABLE,
		ENABLE_BROADCAST,
		POLYPHONY,
		POLYPHONIC,
		CONTROL_BINDING,
		PRESET
	};

	typedef std::vector<SetPortValue*> SetEvents;

	Event*                   _create_event;
	SetEvents                _set_events;
	std::vector<SpecialType> _types;
	std::vector<SpecialType> _remove_types;
	Raul::URI                _subject;
	Resource::Properties     _properties;
	Resource::Properties     _remove;
	Ingen::Resource*         _object;
	GraphImpl*               _graph;
	CompiledGraph*           _compiled_graph;
	LilvState*               _state;
	Resource::Graph          _context;
	ControlBindings::Key     _binding;
	Type                     _type;

	SPtr<ControlBindings::Bindings> _old_bindings;

	std::unique_lock<std::mutex> _poly_lock;  ///< Long-term lock for poly changes
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_SETMETADATA_HPP
