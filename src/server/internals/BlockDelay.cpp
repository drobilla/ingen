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

#include "internals/BlockDelay.hpp"

#include "Buffer.hpp"
#include "InputPort.hpp"
#include "InternalPlugin.hpp"
#include "OutputPort.hpp"

#include "ingen/Forge.hpp"
#include "ingen/URIs.hpp"
#include "raul/Array.hpp"
#include "raul/Maid.hpp"

namespace ingen {
namespace server {

class RunContext;

namespace internals {

InternalPlugin* BlockDelayNode::internal_plugin(URIs& uris) {
	return new InternalPlugin(
		uris, URI(NS_INTERNALS "BlockDelay"), Raul::Symbol("blockDelay"));
}

BlockDelayNode::BlockDelayNode(InternalPlugin*     plugin,
                               BufferFactory&      bufs,
                               const Raul::Symbol& symbol,
                               bool                polyphonic,
                               GraphImpl*          parent,
                               SampleRate          srate)
	: InternalBlock(plugin, symbol, polyphonic, parent, srate)
{
	const ingen::URIs& uris = bufs.uris();
	_ports = bufs.maid().make_managed<Ports>(2);

	_in_port = new InputPort(bufs, this, Raul::Symbol("in"), 0, 1,
	                         PortType::AUDIO, 0, bufs.forge().make(0.0f));
	_in_port->set_property(uris.lv2_name, bufs.forge().alloc("In"));
	_ports->at(0) = _in_port;

	_out_port = new OutputPort(bufs, this, Raul::Symbol("out"), 0, 1,
	                           PortType::AUDIO, 0, bufs.forge().make(0.0f));
	_out_port->set_property(uris.lv2_name, bufs.forge().alloc("Out"));
	_ports->at(1) = _out_port;
}

BlockDelayNode::~BlockDelayNode()
{
	_buffer.reset();
}

void
BlockDelayNode::activate(BufferFactory& bufs)
{
	_buffer = bufs.create(
		bufs.uris().atom_Sound, 0, bufs.audio_buffer_size());

	BlockImpl::activate(bufs);
}

void
BlockDelayNode::run(RunContext& ctx)
{
	// Copy buffer from last cycle to output
	_out_port->buffer(0)->copy(ctx, _buffer.get());

	// Copy input from this cycle to buffer
	_buffer->copy(ctx, _in_port->buffer(0).get());
}

} // namespace internals
} // namespace server
} // namespace ingen
