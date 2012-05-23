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

#ifndef INGEN_EVENTS_SETPORTVALUE_HPP
#define INGEN_EVENTS_SETPORTVALUE_HPP

#include "raul/Atom.hpp"

#include "ControlBindings.hpp"
#include "Event.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class PortImpl;

namespace Events {

/** An event to change the value of a port.
 *
 * This event can either be queued or immediate, depending on the queued
 * parameter passed to the constructor.  It must be passed to the appropriate
 * place (ie queued event passed to the event queue and non-queued event
 * processed in the audio thread) or nasty things will happen.
 *
 * \ingroup engine
 */
class SetPortValue : public Event
{
public:
	SetPortValue(Engine&           engine,
	             Interface*        client,
	             int32_t           id,
	             SampleCount       timestamp,
	             PortImpl*         port,
	             const Raul::Atom& value);

	~SetPortValue();

	bool pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	void apply(Context& context);

	bool                 _queued;
	const Raul::Path     _port_path;
	const Raul::Atom     _value;
	PortImpl*            _port;
	ControlBindings::Key _binding;
};

} // namespace Events
} // namespace Server
} // namespace Ingen

#endif // INGEN_EVENTS_SETPORTVALUE_HPP
