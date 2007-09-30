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

#include "QueuedEvent.hpp"
#include "ThreadManager.hpp"
#include "ProcessContext.hpp"

namespace Ingen {


void
QueuedEvent::pre_process()
{
	assert(ThreadManager::current_thread_id() == THREAD_PRE_PROCESS);
	assert(_pre_processed == false);
	_pre_processed = true;
}


void
QueuedEvent::execute(ProcessContext& context)
{
	assert(_pre_processed);
	assert(_time <= context.end());

	// Didn't prepare in time.  QueuedEvents aren't (necessarily) sample accurate
	// so just run at the beginning of this cycle
	if (_time <= context.start())
		_time = context.start();

	Event::execute(context);
}
	

} // namespace Ingen

