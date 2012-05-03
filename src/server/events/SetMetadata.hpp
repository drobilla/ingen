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

#include <glibmm/thread.h>

#include <vector>

#include "raul/URI.hpp"

#include "ControlBindings.hpp"
#include "Event.hpp"
#include "ingen/shared/ResourceImpl.hpp"

namespace Ingen {
namespace Server {

class GraphObjectImpl;
class PatchImpl;
class CompiledPatch;

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
class SetMetadata : public Event
{
public:
	SetMetadata(Engine&                     engine,
	            Interface*                  client,
	            int32_t                     id,
	            SampleCount                 timestamp,
	            bool                        create,
	            Resource::Graph             context,
	            const Raul::URI&            subject,
	            const Resource::Properties& properties,
	            const Resource::Properties& remove = Resource::Properties());

	~SetMetadata();

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	enum SpecialType {
		NONE,
		ENABLE,
		ENABLE_BROADCAST,
		POLYPHONY,
		POLYPHONIC,
		CONTROL_BINDING
	};

	typedef std::vector<SetPortValue*> SetEvents;

	Event*                       _create_event;
	SetEvents                    _set_events;
	std::vector<SpecialType>     _types;
	std::vector<SpecialType>     _remove_types;
	Raul::URI                    _subject;
	Resource::Properties         _properties;
	Resource::Properties         _remove;
	Ingen::Shared::ResourceImpl* _object;
	PatchImpl*                   _patch;
	CompiledPatch*               _compiled_patch;
	std::string                  _error_predicate;
	bool                         _create;
	Resource::Graph              _context;
	ControlBindings::Key         _binding;

	SharedPtr<ControlBindings::Bindings> _old_bindings;

	Glib::RWLock::WriterLock _lock;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_SETMETADATA_HPP
