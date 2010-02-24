/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef INGEN_INTERNALS_DELAY_HPP
#define INGEN_INTERNALS_DELAY_HPP

#include <string>
#include <math.h>
#include "types.hpp"
//#include "Buffer.hpp"
//#include "BufferFactory.hpp"
#include "NodeImpl.hpp"

namespace Ingen {

class InputPort;
class OutputPort;
class InternalPlugin;
class BufferFactory;

namespace Internals {


/** MIDI note input node.
 *
 * For pitched instruments like keyboard, etc.
 *
 * \ingroup engine
 */
class DelayNode : public NodeImpl
{
public:
	DelayNode(BufferFactory& bufs, const std::string& path, bool polyphonic, PatchImpl* parent, SampleRate srate);
	~DelayNode();

	void activate(BufferFactory& bufs);

	void process(ProcessContext& context);

	static InternalPlugin& internal_plugin();

	float delay_samples() const { return _delay_samples; }

private:
	inline float& buffer_at(long phase) const { return _buffer[phase & _buffer_mask]; }

	InputPort*  _delay_port;
	InputPort*  _in_port;
	OutputPort* _out_port;

	typedef long Phase;

	float*   _buffer;
	uint32_t _buffer_length;
	uint32_t _buffer_mask;
	Phase    _write_phase;
	float    _last_delay_time;
	float    _delay_samples;
};


} // namespace Ingen
} // namespace Internals

#endif // INGEN_INTERNALS_DELAY_HPP
