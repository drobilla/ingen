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

#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include <pthread.h>
#include "types.hpp"
#include <raul/SRSWQueue.hpp>
#include <raul/Slave.hpp>

namespace Raul { class Maid; }

namespace Ingen {

class Event;


/** Processor for Events after leaving the audio thread.
 *
 * The audio thread pushes events to this when it is done with them (which
 * is realtime-safe), which signals the processing thread through a semaphore
 * to handle the event and pass it on to the Maid.
 *
 * \ingroup engine
 */
class PostProcessor : public Raul::Slave
{
public:
	PostProcessor(Raul::Maid& maid, size_t queue_size);

	/** Push an event on to the process queue, realtime-safe, not thread-safe. */
	inline void push(Event* const ev) { _events.push(ev); }

private:
	Raul::Maid&             _maid;
	Raul::SRSWQueue<Event*> _events;
	virtual void            _whipped();
};


} // namespace Ingen

#endif // POSTPROCESSOR_H
