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

#include "ingen/URIs.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#include "Buffer.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "ProcessContext.hpp"
#include "internals/Time.hpp"
#include "util.hpp"

namespace Ingen {
namespace Server {
namespace Internals {

InternalPlugin* TimeNode::internal_plugin(URIs& uris) {
	return new InternalPlugin(
		uris, Raul::URI(NS_INTERNALS "Time"), Raul::Symbol("time"));
}

TimeNode::TimeNode(InternalPlugin*     plugin,
                   BufferFactory&      bufs,
                   const Raul::Symbol& symbol,
                   bool                polyphonic,
                   GraphImpl*          parent,
                   SampleRate          srate)
	: BlockImpl(plugin, symbol, false, parent, srate)
{
	const Ingen::URIs& uris = bufs.uris();
	_ports = new Raul::Array<PortImpl*>(1);

	_notify_port = new OutputPort(
		bufs, this, Raul::Symbol("notify"), 0, 1,
		PortType::ATOM, uris.atom_Sequence, Atom(), 1024);
	_notify_port->set_property(uris.lv2_name, bufs.forge().alloc("Notify"));
	_notify_port->set_property(uris.atom_supports,
	                           bufs.forge().make_urid(uris.time_Position));
	_ports->at(0) = _notify_port;
}

void
TimeNode::run(ProcessContext& context)
{
	BufferRef          buf = _notify_port->buffer(0);
	LV2_Atom_Sequence* seq = (LV2_Atom_Sequence*)buf->atom();

	// Initialise output to the empty sequence
	seq->atom.type = _notify_port->bufs().uris().atom_Sequence;
	seq->atom.size = sizeof(LV2_Atom_Sequence_Body);
	seq->body.unit = 0;
	seq->body.pad  = 0;

	// Ask the driver to append any time events for this cycle
	context.engine().driver()->append_time_events(
		context, *_notify_port->buffer(0));
}

} // namespace Internals
} // namespace Server
} // namespace Ingen
