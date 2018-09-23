/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

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

#include "ingen/Atom.hpp"

#include "BufferRef.hpp"
#include "ControlBindings.hpp"
#include "Event.hpp"
#include "types.hpp"

namespace ingen {
namespace server {

class PortImpl;

namespace events {

/** An event to change the value of a port.
 *
 * \ingroup engine
 */
class SetPortValue : public Event
{
public:
	SetPortValue(Engine&         engine,
	             SPtr<Interface> client,
	             int32_t         id,
	             SampleCount     timestamp,
	             PortImpl*       port,
	             const Atom&     value,
	             bool            activity,
	             bool            synthetic = false);

	bool pre_process(PreProcessContext& ctx) override;
	void execute(RunContext& context) override;
	void post_process() override;

	bool synthetic() const { return _synthetic; }

private:
	void apply(RunContext& context);

	PortImpl*            _port;
	const Atom           _value;
	BufferRef            _buffer;
	ControlBindings::Key _binding;
	bool                 _activity;
	bool                 _synthetic;
};

} // namespace events
} // namespace server
} // namespace ingen

#endif // INGEN_EVENTS_SETPORTVALUE_HPP
