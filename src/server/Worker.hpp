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

#ifndef INGEN_ENGINE_WORKER_HPP
#define INGEN_ENGINE_WORKER_HPP

#include "ingen/LV2Features.hpp"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "raul/RingBuffer.hpp"
#include "raul/Semaphore.hpp"
#include "raul/Thread.hpp"

namespace Ingen {

class Log;

namespace Server {

class LV2Block;

class Worker : public Raul::Thread
{
public:
	Worker(Log& log, uint32_t buffer_size);
	~Worker();

	struct Schedule : public LV2Features::Feature {
		SharedPtr<LV2_Feature> feature(World* world, GraphObject* n);
	};

	LV2_Worker_Status request(LV2Block*   block,
	                          uint32_t    size,
	                          const void* data);

	SharedPtr<Schedule> schedule_feature() { return _schedule; }

private:
	SharedPtr<Schedule> _schedule;

	Log&             _log;
	Raul::Semaphore  _sem;
	Raul::RingBuffer _requests;
	Raul::RingBuffer _responses;
	uint8_t* const   _buffer;
	const uint32_t   _buffer_size;

	virtual void _run();
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_WORKER_HPP
