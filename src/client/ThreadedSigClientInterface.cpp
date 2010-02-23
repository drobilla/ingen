/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "raul/log.hpp"
#include "common/interface/Patch.hpp"
#include "common/interface/Plugin.hpp"
#include "common/interface/Port.hpp"
#include "ThreadedSigClientInterface.hpp"

using namespace std;
using namespace Raul;

namespace Ingen {
namespace Client {


/** Push an event (from the engine, ie 'new patch') on to the queue.
 */
void
ThreadedSigClientInterface::push_sig(Closure ev)
{
	bool success = false;
	while (!success) {
		success = _sigs.push(ev);
		if (!success) {
			warn << "Client event queue full.  Waiting..." << endl;
			_mutex.lock();
			_cond.wait(_mutex);
			_mutex.unlock();
			warn << "Queue drained, continuing" << endl;
		}
	}
}


/** Process all queued events that came from the OSC thread.
 *
 * This function should be called from the Gtk thread to emit signals and cause
 * the connected methods to execute.
 */
bool
ThreadedSigClientInterface::emit_signals()
{
	// Process a limited number of events, to prevent locking the GTK
	// thread indefinitely while processing continually arriving events

	size_t num_processed = 0;
	while (!_sigs.empty() && num_processed++ < (_sigs.capacity() * 3 / 4)) {
		Closure& ev = _sigs.front();
		ev();
		ev.disconnect();
		_sigs.pop();
	}

	_mutex.lock();
	_cond.broadcast();
	_mutex.unlock();

	return true;
}


} // namespace Client
} // namespace Ingen
