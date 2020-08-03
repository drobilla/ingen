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

#include "Worker.hpp"

#include "Engine.hpp"
#include "GraphImpl.hpp"
#include "LV2Block.hpp"

#include "ingen/Log.hpp"
#include "ingen/Node.hpp"
#include "ingen/memory.hpp"
#include "lv2/core/lv2.h"
#include "lv2/worker/worker.h"

#include <cstdlib>
#include <memory>

namespace ingen {

class World;

namespace server {

/// A message in the Worker::_requests ring
struct MessageHeader {
	LV2Block* block;  ///< Node this message is from
	uint32_t  size;  ///< Size of following data
	// `size' bytes of data follow here
};

static LV2_Worker_Status
schedule(LV2_Worker_Schedule_Handle handle,
         uint32_t                   size,
         const void*                data)
{
	auto*   block  = static_cast<LV2Block*>(handle);
	Engine& engine = block->parent_graph()->engine();

	return engine.worker()->request(block, size, data);
}

static LV2_Worker_Status
schedule_sync(LV2_Worker_Schedule_Handle handle,
              uint32_t                   size,
              const void*                data)
{
	auto*   block  = static_cast<LV2Block*>(handle);
	Engine& engine = block->parent_graph()->engine();

	return engine.sync_worker()->request(block, size, data);
}

LV2_Worker_Status
Worker::request(LV2Block*   block,
                uint32_t    size,
                const void* data)
{
	if (_synchronous) {
		return block->work(size, data);
	}

	Engine& engine = block->parent_graph()->engine();
	if (_requests.write_space() < sizeof(MessageHeader) + size) {
		engine.log().error("Work request ring overflow\n");
		return LV2_WORKER_ERR_NO_SPACE;
	}

	const MessageHeader msg = { block, size };
	if (_requests.write(sizeof(msg), &msg) != sizeof(msg)) {
		engine.log().error("Error writing header to work request ring\n");
		return LV2_WORKER_ERR_UNKNOWN;
	}
	if (_requests.write(size, data) != size) {
		engine.log().error("Error writing body to work request ring\n");
		return LV2_WORKER_ERR_UNKNOWN;
	}

	_sem.post();

	return LV2_WORKER_SUCCESS;
}

std::shared_ptr<LV2_Feature>
Worker::Schedule::feature(World&, Node* n)
{
	auto* block = dynamic_cast<LV2Block*>(n);
	if (!block) {
		return nullptr;
	}

	auto* data = static_cast<LV2_Worker_Schedule*>(malloc(sizeof(LV2_Worker_Schedule)));

	data->handle        = block;
	data->schedule_work = synchronous ? schedule_sync : schedule;

	auto* f = static_cast<LV2_Feature*>(malloc(sizeof(LV2_Feature)));
	f->URI  = LV2_WORKER__schedule;
	f->data = data;

	return std::shared_ptr<LV2_Feature>(f, &free_feature);
}

Worker::Worker(Log& log, uint32_t buffer_size, bool synchronous)
	: _schedule(new Schedule(synchronous))
	, _log(log)
	, _sem(0)
	, _requests(buffer_size)
	, _responses(buffer_size)
	, _buffer(static_cast<uint8_t*>(malloc(buffer_size)))
	, _buffer_size(buffer_size)
	, _thread(nullptr)
	, _exit_flag(false)
	, _synchronous(synchronous)
{
	if (!synchronous) {
		_thread = make_unique<std::thread>(&Worker::run, this);
	}
}

Worker::~Worker()
{
	_exit_flag = true;
	_sem.post();
	if (_thread) {
		_thread->join();
	}
	free(_buffer);
}

void
Worker::run()
{
	while (_sem.wait() && !_exit_flag) {
		MessageHeader msg{};
		if (_requests.read_space() > sizeof(msg)) {
			if (_requests.read(sizeof(msg), &msg) != sizeof(msg)) {
				_log.error("Error reading header from work request ring\n");
				continue;
			}

			if (msg.size >= _buffer_size - sizeof(msg)) {
				_log.error("Corrupt work request ring\n");
				return;
			}

			if (_requests.read(msg.size, _buffer) != msg.size) {
				_log.error("Error reading body from work request ring\n");
				continue;
			}

			msg.block->work(msg.size, _buffer);
		}
	}
}

} // namespace server
} // namespace ingen
