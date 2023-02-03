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

#include "internals/Time.hpp"

#include "BlockImpl.hpp"
#include "Buffer.hpp"
#include "BufferFactory.hpp"
#include "BufferRef.hpp"
#include "Driver.hpp"
#include "Engine.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"
#include "PortType.hpp"
#include "RunContext.hpp"

#include "ingen/Atom.hpp"
#include "ingen/Forge.hpp"
#include "ingen/URI.hpp"
#include "ingen/URIs.hpp"
#include "lv2/atom/atom.h"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"
#include "raul/Symbol.hpp"

#include <memory>

namespace ingen::server {

class GraphImpl;

namespace internals {

InternalPlugin* TimeNode::internal_plugin(URIs& uris) {
	return new InternalPlugin(
		uris, URI(NS_INTERNALS "Time"), raul::Symbol("time"));
}

TimeNode::TimeNode(InternalPlugin*     plugin,
                   BufferFactory&      bufs,
                   const raul::Symbol& symbol,
                   bool                polyphonic,
                   GraphImpl*          parent,
                   SampleRate          srate)
	: InternalBlock(plugin, symbol, false, parent, srate)
{
	const ingen::URIs& uris = bufs.uris();
	_ports = bufs.maid().make_managed<Ports>(1);

	_notify_port = new OutputPort(
		bufs, this, raul::Symbol("notify"), 0, 1,
		PortType::ATOM, uris.atom_Sequence, Atom(), 1024);
	_notify_port->set_property(uris.lv2_name, bufs.forge().alloc("Notify"));
	_notify_port->set_property(uris.atom_supports,
	                           bufs.forge().make_urid(uris.time_Position));
	_ports->at(0) = _notify_port;
}

void
TimeNode::run(RunContext& ctx)
{
	const BufferRef buf = _notify_port->buffer(0);
	auto* const     seq = buf->get<LV2_Atom_Sequence>();

	// Initialise output to the empty sequence
	seq->atom.type = _notify_port->bufs().uris().atom_Sequence;
	seq->atom.size = sizeof(LV2_Atom_Sequence_Body);
	seq->body.unit = 0;
	seq->body.pad  = 0;

	// Ask the driver to append any time events for this cycle
	ctx.engine().driver()->append_time_events(ctx, *_notify_port->buffer(0));
}

} // namespace internals
} // namespace ingen::server
