/* This file is part of Om. Copyright (C) 2006 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "ThreadedSigClientInterface.h"
#include <iostream>
using std::cerr; using std::endl;

namespace LibOmClient {


/** Push an event (from the engine, ie 'new patch') on to the queue.
 */
void
ThreadedSigClientInterface::push_sig(Closure ev)
{
	cerr << "-- pushing event\n";

	bool success = false;
	bool first   = true;
	
	// (Very) slow busy-wait if the queue is full
	// FIXME: Make this wait on a signal from process_sigs iff this happens
	while (!success) {
		success = _sigs.push(ev);
		if (!success) {
			if (first) {
				cerr << "[ThreadedSigClientInterface] WARNING:  (Client) event queue full.  Waiting to try again..." << endl;
				first = false;
			}
			usleep(200000); // 100 milliseconds (2* rate process_sigs is called)
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
	// Process a maximum of queue-size events, to prevent locking the GTK
	// thread indefinitely while processing continually arriving events
	size_t num_processed = 0;
	while (!_sigs.is_empty() && num_processed++ < _sigs.capacity()/2) {
		cerr << "-- emitting event\n";
		Closure& ev = _sigs.pop();
		ev();
		ev.disconnect();
	}

	return true;
}


} // namespace LibOmClient
