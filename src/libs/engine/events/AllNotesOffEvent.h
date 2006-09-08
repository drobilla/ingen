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

#ifndef ALLNOTESOFFEVENT_H
#define ALLNOTESOFFEVENT_H

#include "Event.h"
#include <string>
using std::string;

namespace Ingen {

class Patch;


/** A note off event for all active voices.
 *
 * \ingroup engine
 */
class AllNotesOffEvent : public Event
{
public:
	AllNotesOffEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, Patch* patch);
	AllNotesOffEvent(Engine& engine, CountedPtr<Responder> responder, SampleCount timestamp, const string& patch_path);
	
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	Patch* m_patch;
	string m_patch_path;
};


} // namespace Ingen

#endif // ALLNOTESOFFEVENT_H
