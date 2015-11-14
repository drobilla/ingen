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

#ifndef INGEN_INTERNALS_BLOCKDELAY_HPP
#define INGEN_INTERNALS_BLOCKDELAY_HPP

#include "BufferRef.hpp"
#include "InternalBlock.hpp"
#include "types.hpp"

namespace Ingen {
namespace Server {

class InputPort;
class OutputPort;
class InternalPlugin;
class BufferFactory;

namespace Internals {

class BlockDelayNode : public InternalBlock
{
public:
	BlockDelayNode(InternalPlugin*     plugin,
	               BufferFactory&      bufs,
	               const Raul::Symbol& symbol,
	               bool                polyphonic,
	               GraphImpl*          parent,
	               SampleRate          srate);

	~BlockDelayNode();

	void activate(BufferFactory& bufs);

	void run(RunContext& context);

	static InternalPlugin* internal_plugin(URIs& uris);

private:
	InputPort*  _in_port;
	OutputPort* _out_port;
	BufferRef   _buffer;
};

} // namespace Server
} // namespace Ingen
} // namespace Internals

#endif // INGEN_INTERNALS_BLOCKDELAY_HPP
