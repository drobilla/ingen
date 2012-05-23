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

#include "Engine.hpp"
#include "ProcessContext.hpp"
#include "ProcessSlave.hpp"

namespace Ingen {
namespace Server {

ProcessContext::ProcessContext(Engine& engine)
	: Context(engine, engine.event_queue_size(), AUDIO)
{}

void
ProcessContext::activate(uint32_t parallelism, bool sched_rt)
{
	for (uint32_t i = 0; i < _slaves.size(); ++i) {
		delete _slaves[i];
	}
	_slaves.clear();
	_slaves.reserve(parallelism);
	for (uint32_t i = 0; i < parallelism - 1; ++i) {
		_slaves.push_back(new ProcessSlave(_engine, sched_rt));
	}
}

} // namespace Server
} // namespace Ingen
