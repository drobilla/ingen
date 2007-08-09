/* This file is part of Ingen.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef PROCESS_SLAVE_HPP
#define PROCESS_SLAVE_HPP

#include CONFIG_H_PATH

#include <sstream>
#include <raul/Slave.hpp>
#include <raul/Array.hpp>
#include <raul/AtomicInt.hpp>
#include "types.hpp"

namespace Ingen {

class Node;
class CompiledPatch;


class ProcessSlave : protected Raul::Slave {
public:
	ProcessSlave(bool realtime)
			: _id(_next_id++), _state(STATE_FINISHED), _index(0), _compiled_patch(NULL)
	{
		std::stringstream ss;
		ss << "Process Slave ";
		ss << _id;
		set_name(ss.str());
		
		start();
		
		if (realtime)
			set_scheduling(SCHED_FIFO, 40);
	}
	
	~ProcessSlave() {
		stop();
	}

	inline void whip(CompiledPatch* compiled_patch, size_t start_index,
			SampleCount nframes, FrameTime start, FrameTime end) {
		assert(_state == STATE_FINISHED);
		_nframes = nframes;
		_start = start;
		_end = end;
		_index = start_index;
		_compiled_patch = compiled_patch;
		_state = STATE_RUNNING;
		Raul::Slave::whip();
	}

	inline void finish() {
		while (_state.get() != STATE_FINISHED)
			_state.compare_and_exchange(STATE_RUNNING, STATE_FINISH_SIGNALLED);
	}

	size_t id() const { return _id; }
	
private:

	void _whipped();

	static size_t _next_id;

	static const int STATE_RUNNING = 0;
	static const int STATE_FINISH_SIGNALLED = 1;
	static const int STATE_FINISHED = 2;

	size_t           _id;
	Raul::AtomicInt  _state;
	SampleCount      _nframes;
	FrameTime        _start;
	FrameTime        _end;
	size_t           _index;
	CompiledPatch*   _compiled_patch;
};


} // namespace Ingen

#endif // PROCESS_SLAVE_HPP

