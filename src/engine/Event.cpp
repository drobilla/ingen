/* This file is part of Ingen.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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


void
Event::execute(ProcessContext& context)
{
	ThreadManager::assert_thread(THREAD_PROCESS);
	assert(!_executed);
	assert(_time <= context.end());

	// Missed the event, jitter, damnit.
	if (_time < context.start())
		_time = context.start();

	_executed = true;
}


void
Event::post_process()
{
	ThreadManager::assert_not_thread(THREAD_PROCESS);
}


} // namespace Ingen

