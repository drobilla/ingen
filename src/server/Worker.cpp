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

#include "ingen/LV2Features.hpp"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "raul/log.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "LV2Node.hpp"
#include "PatchImpl.hpp"
#include "Worker.hpp"

namespace Ingen {
namespace Server {

/// A message in the Worker::_requests ring
struct MessageHeader {
	LV2Node* node;  ///< Node this message is from
	uint32_t size;  ///< Size of following data
	// `size' bytes of data follow here
};

static LV2_Worker_Status
schedule(LV2_Worker_Schedule_Handle handle,
         uint32_t                   size,
         const void*                data)
{
	LV2Node* node   = (LV2Node*)handle;
	Engine&  engine = node->parent_patch()->engine();
	Worker*  worker = engine.worker();

	return worker->request(node, size, data);
}

LV2_Worker_Status
Worker::request(LV2Node*    node,
                uint32_t    size,
                const void* data)
{
	if (_requests.write_space() < sizeof(MessageHeader) + size) {
		Raul::error("Work request ring overflow\n");
		return LV2_WORKER_ERR_NO_SPACE;
	}

	const MessageHeader msg = { node, size };
	if (_requests.write(sizeof(msg), &msg) != sizeof(msg)) {
		Raul::error("Error writing header to work request ring\n");
		return LV2_WORKER_ERR_UNKNOWN;
	}
	if (_requests.write(size, data) != size) {
		Raul::error("Error writing body to work request ring\n");
		return LV2_WORKER_ERR_UNKNOWN;
	}

	_sem.post();

	return LV2_WORKER_SUCCESS;
}

static void
delete_feature(LV2_Feature* feature)
{
	free(feature->data);
	free(feature);
}

SharedPtr<LV2_Feature>
Worker::Schedule::feature(World* world, GraphObject* n)
{
	LV2Node* node = dynamic_cast<LV2Node*>(n);
	if (!node) {
		return SharedPtr<LV2_Feature>();
	}

	LV2_Worker_Schedule* data = (LV2_Worker_Schedule*)malloc(
		sizeof(LV2_Worker_Schedule));
	data->handle        = node;
	data->schedule_work = schedule;

	LV2_Feature* f = (LV2_Feature*)malloc(sizeof(LV2_Feature));
	f->URI  = LV2_WORKER__schedule;
	f->data = data;

	return SharedPtr<LV2_Feature>(f, &delete_feature);

}

Worker::Worker(uint32_t buffer_size)
	: Raul::Thread("Worker")
	, _schedule(new Schedule())
	, _sem(0)
	, _requests(buffer_size)
	, _responses(buffer_size)
	, _buffer((uint8_t*)malloc(buffer_size))
	, _buffer_size(buffer_size)
{
	start();
}

Worker::~Worker()
{
	free(_buffer);
}

void
Worker::_run()
{
	while (_sem.wait() && !_exit_flag) {
		MessageHeader msg;
		if (_requests.read_space() > sizeof(msg)) {
			if (_requests.read(sizeof(msg), &msg) != sizeof(msg)) {
				Raul::error("Error reading header from work request ring\n");
				continue;
			}

			if (msg.size >= _buffer_size - sizeof(msg)) {
				Raul::error("Corrupt work request ring\n");
				return;
			}

			if (_requests.read(msg.size, _buffer) != msg.size) {
				Raul::error("Error reading body from work request ring\n");
				continue;
			}

			msg.node->work(msg.size, _buffer);
		}
	}
}

} // namespace Server
} // namespace Ingen
