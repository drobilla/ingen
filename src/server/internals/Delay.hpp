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

#ifndef INGEN_INTERNALS_DELAY_HPP
#define INGEN_INTERNALS_DELAY_HPP

#include <string>
#include <math.h>
#include "types.hpp"
#include "NodeImpl.hpp"

namespace Ingen {
namespace Server {

class InputPort;
class OutputPort;
class InternalPlugin;
class BufferFactory;

namespace Internals {

class DelayNode : public NodeImpl
{
public:
	DelayNode(InternalPlugin*    plugin,
	          BufferFactory&     bufs,
	          const std::string& path,
	          bool               polyphonic,
	          PatchImpl*         parent,
	          SampleRate         srate);

	~DelayNode();

	void activate(BufferFactory& bufs);

	void process(ProcessContext& context);

	static InternalPlugin* internal_plugin(URIs& uris);

	float delay_samples() const { return _delay_samples; }

private:
	inline float& buffer_at(int64_t phase) const { return _buffer[phase & _buffer_mask]; }

	InputPort*  _delay_port;
	InputPort*  _in_port;
	OutputPort* _out_port;
	float*      _buffer;
	uint32_t    _buffer_length;
	uint32_t    _buffer_mask;
	uint64_t    _write_phase;
	float       _last_delay_time;
	float       _delay_samples;
};

} // namespace Server
} // namespace Ingen
} // namespace Internals

#endif // INGEN_INTERNALS_DELAY_HPP
