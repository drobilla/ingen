/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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

#include "PostProcessor.h"
#include <cassert>
#include <iostream>
#include <pthread.h>
#include "Event.h"
#include "raul/Queue.h"
#include "Maid.h"


using std::cerr; using std::cout; using std::endl;

namespace Ingen {


PostProcessor::PostProcessor(Maid& maid, size_t queue_size)
: _maid(maid),
  _events(queue_size)
{
	set_name("PostProcessor");
}


/** Post-Process every pending event.
 *
 * The PostProcessor should be whipped by the audio thread once every cycle
 */
void
PostProcessor::_whipped()
{
	while ( ! _events.is_empty()) {
		Event* const ev = _events.pop();
		assert(ev);
		ev->post_process();
		_maid.push(ev);
	}
}

} // namespace Ingen
