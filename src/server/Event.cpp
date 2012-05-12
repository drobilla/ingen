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

#include "ingen/Interface.hpp"

#include "Driver.hpp"
#include "Engine.hpp"
#include "Event.hpp"
#include "ProcessContext.hpp"
#include "ThreadManager.hpp"

/*! \page methods Method Documentation
 *
 * <p>All changes in Ingen (both engine and client) occur as a result of
 * a small set of methods defined in terms of RDF and matching the
 * HTTP and WebDAV standards as closely as possible.</p>
 */

namespace Ingen {
namespace Server {

void
Event::pre_process()
{
	ThreadManager::assert_thread(THREAD_PRE_PROCESS);
	assert(_pre_processed == false);
	_pre_processed = true;
}

void
Event::execute(ProcessContext& context)
{
	assert(_pre_processed);
	assert(!_executed);
	assert(_time <= context.end());

	// Didn't get around to executing in time, jitter, oh well...
	if (_time < context.start())
		_time = context.start();

	_executed = true;
}

void
Event::post_process()
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);
}

void
Event::respond(Status status)
{
	if (_request_client) {
		_request_client->response(_request_id, status);
	}
}

} // namespace Server
} // namespace Ingen
