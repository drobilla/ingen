/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "PostProcessor.h"
#include <cassert>
#include <iostream>
#include <pthread.h>
#include "Om.h"
#include "OmApp.h"
#include "Event.h"
#include "util/Queue.h"
#include "Maid.h"


using std::cerr; using std::cout; using std::endl;

namespace Om {


PostProcessor::PostProcessor(size_t queue_size)
: _events(queue_size)
{
	set_name("PostProcessor");
}


/** Post processing thread.
 *
 * Infinite loop that waits on the semaphore and processes every enqueued
 * event (to be signalled at the end of every process cycle).
 */
void
PostProcessor::_signalled()
{
	while ( ! _events.is_empty()) {
		Event* const ev = _events.pop();
		assert(ev);
		ev->post_process();
		om->maid()->push(ev);
	}
}

} // namespace Om
