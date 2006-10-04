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

#ifndef DEACTIVATEEVENT_H
#define DEACTIVATEEVENT_H

#include "QueuedEvent.h"

namespace Ingen {


/** Deactivates the engine.
 *
 * \ingroup engine
 */
class DeactivateEvent : public QueuedEvent
{
public:
	DeactivateEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp);
	
	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();
};


} // namespace Ingen

#endif // DEACTIVATEEVENT_H
