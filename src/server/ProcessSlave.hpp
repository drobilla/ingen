/* This file is part of Ingen.
 * Copyright 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef INGEN_ENGINE_PROCESSSLAVE_HPP
#define INGEN_ENGINE_PROCESSSLAVE_HPP

#include <sstream>

#include "raul/Array.hpp"
#include "raul/AtomicInt.hpp"
#include "raul/Slave.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "ProcessContext.hpp"

namespace Ingen {
namespace Server {

class NodeImpl;
class CompiledPatch;

class ProcessSlave : protected Raul::Slave {
public:
	ProcessSlave(Engine& engine, bool realtime)
		: _engine(engine)
		, _id(_next_id++)
		, _index(0)
		, _state(STATE_FINISHED)
		, _compiled_patch(NULL)
		, _context(NULL)
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

	inline void whip(CompiledPatch*  compiled_patch,
	                 uint32_t        start_index,
	                 ProcessContext& context)
	{
		assert(_state == STATE_FINISHED);
		_index          = start_index;
		_state          = STATE_RUNNING;
		_compiled_patch = compiled_patch;
		_context        = &context;

		Raul::Slave::whip();
	}

	inline void finish() {
		while (_state.get() != STATE_FINISHED)
			_state.compare_and_exchange(STATE_RUNNING, STATE_FINISH_SIGNALLED);
	}

	inline uint32_t              id()      const { return _id; }
	inline const ProcessContext& context() const { return _engine.driver()->context(); }
	inline       ProcessContext& context()       { return _engine.driver()->context(); }

private:
	void _whipped();

	static uint32_t _next_id;

	static const int STATE_RUNNING          = 0;
	static const int STATE_FINISH_SIGNALLED = 1;
	static const int STATE_FINISHED         = 2;

	Engine&         _engine;
	uint32_t        _id;
	uint32_t        _index;
	Raul::AtomicInt _state;
	CompiledPatch*  _compiled_patch;
	ProcessContext* _context;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PROCESSSLAVE_HPP