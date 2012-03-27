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

#ifndef INGEN_EVENTS_CREATEPORT_HPP
#define INGEN_EVENTS_CREATEPORT_HPP

#include "ingen/Resource.hpp"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "raul/Array.hpp"
#include "raul/Path.hpp"

#include "Event.hpp"
#include "PortType.hpp"

namespace Ingen {
namespace Server {

class PatchImpl;
class PortImpl;
class DriverPort;

namespace Events {

/** An event to add a Port to a Patch.
 *
 * \ingroup engine
 */
class CreatePort : public Event
{
public:
	CreatePort(Engine&                     engine,
	           Interface*                  client,
	           int32_t                     id,
	           SampleCount                 timestamp,
	           const Raul::Path&           path,
	           bool                        is_output,
	           const Resource::Properties& properties);

	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	Raul::Path              _path;
	Raul::URI               _type;
	PortType                _port_type;
	LV2_URID                _buffer_type;
	PatchImpl*              _patch;
	PortImpl*               _patch_port;
	Raul::Array<PortImpl*>* _ports_array; ///< New (external) ports array for Patch
	DriverPort*             _driver_port; ///< Driver (eg Jack) port if this is a toplevel port
	Resource::Properties    _properties;
	bool                    _is_output;
};

} // namespace Server
} // namespace Ingen
} // namespace Events

#endif // INGEN_EVENTS_CREATEPORT_HPP
