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

#ifndef INGEN_ENGINE_PROCESSSLAVE_HPP
#define INGEN_ENGINE_PROCESSSLAVE_HPP

#include <stdint.h>

#include "raul/AtomicInt.hpp"
#include "raul/Slave.hpp"
#include "raul/log.hpp"

namespace Ingen {
namespace Server {

class CompiledPatch;
class Engine;
class ProcessContext;

class ProcessSlave : protected Raul::Slave {
public:
	ProcessSlave(Engine& engine, bool realtime)
		: Raul::Slave((Raul::fmt("Process Slave %1%") % _next_id).str())
		, _engine(engine)
		, _context(NULL)
		, _id(_next_id++)
		, _index(0)
		, _state(STATE_FINISHED)
		, _compiled_patch(NULL)
	{
		start();

		if (realtime) {
			set_scheduling(true, 40);
		}
	}

	~ProcessSlave() {
		stop();
	}

	inline void whip(CompiledPatch*  compiled_patch,
	                 uint32_t        start_index,
	                 ProcessContext& context)
	{
		assert(_state == STATE_FINISHED);
		_context        = &context;
		_index          = start_index;
		_state          = STATE_RUNNING;
		_compiled_patch = compiled_patch;

		Raul::Slave::whip();
	}

	inline void finish() {
		while (_state.get() != STATE_FINISHED)
			_state.compare_and_exchange(STATE_RUNNING, STATE_FINISH_SIGNALLED);
	}

	inline uint32_t id() const { return _id; }

private:
	void _whipped();

	static uint32_t _next_id;

	static const int STATE_RUNNING          = 0;
	static const int STATE_FINISH_SIGNALLED = 1;
	static const int STATE_FINISHED         = 2;

	Engine&         _engine;
	ProcessContext* _context;
	uint32_t        _id;
	uint32_t        _index;
	Raul::AtomicInt _state;
	CompiledPatch*  _compiled_patch;
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_PROCESSSLAVE_HPP
