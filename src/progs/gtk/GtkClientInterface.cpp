/* This file is part of Om.  Copyright (C) 2006 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "GtkClientInterface.h"
#include <cstdio>
#include <string>
#include <sys/time.h>
#include <time.h>
#include <gtkmm.h>
#include "ControlInterface.h"

namespace OmGtk {


GtkClientInterface::GtkClientInterface(ControlInterface* interface, int client_port)
: OSCListener(client_port)
, ModelClientInterface()
, _interface(interface)
, _num_plugins(0)
, _events(4096)
{
}


/** Push an event (from the engine, ie 'new patch') on to the queue.
 */
void
GtkClientInterface::push_event(Closure ev)
{
	bool success = false;
	bool first   = true;
	
	// (Very) slow busy-wait if the queue is full
	// FIXME: Make this wait on a signal from process_events iff this happens
	while (!success) {
		success = _events.push(ev);
		if (!success) {
			if (first) {
				cerr << "[GtkClientInterface] WARNING:  (Client) event queue full.  Waiting to try again..." << endl;
				first = false;
			}
			usleep(200000); // 100 milliseconds (2* rate process_events is called)
		}
	}
}


/** Process all queued events that came from the OSC thread.
 *
 * This function is to be called from the Gtk thread only.
 */
bool
GtkClientInterface::process_events()
{
	// Process a maximum of queue-size events, to prevent locking the GTK
	// thread indefinitely while processing continually arriving events
	size_t num_processed = 0;
	while (!_events.is_empty() && num_processed++ < _events.capacity()/2) {
		Closure& ev = _events.pop();
		ev();
		ev.disconnect();
	}

	return true;
}


} // namespace OmGtk
