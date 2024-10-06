/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

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
#include "lv2/core/lv2.h"
#include "lv2/worker/worker.h"
#include "raul/RingBuffer.hpp"
#include "raul/Semaphore.hpp"

#include <cstdint>
#include <memory>
#include <thread>

namespace ingen {

class Log;
class Node;
class World;

namespace server {

class LV2Block;

class Worker
{
public:
	Worker(Log& log, uint32_t buffer_size, bool synchronous=false);
	~Worker();

	struct Schedule : public LV2Features::Feature {
		explicit Schedule(bool sync) noexcept : synchronous(sync) {}

		const char* uri() const override { return LV2_WORKER__schedule; }

		std::shared_ptr<LV2_Feature> feature(World& world, Node* n) override;

		const bool synchronous;
	};

	LV2_Worker_Status request(LV2Block*   block,
	                          uint32_t    size,
	                          const void* data);

	std::shared_ptr<Schedule> schedule_feature() { return _schedule; }

private:
	std::shared_ptr<Schedule> _schedule;

	Log&                         _log;
	raul::Semaphore              _sem{0};
	raul::RingBuffer             _requests;
	raul::RingBuffer             _responses;
	uint8_t* const               _buffer;
	const uint32_t               _buffer_size;
	std::unique_ptr<std::thread> _thread;
	bool                         _exit_flag{false};
	bool                         _synchronous;

	void run();
};

} // namespace server
} // namespace ingen

#endif // INGEN_ENGINE_WORKER_HPP
