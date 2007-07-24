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

#ifndef DISABLEPATCHEVENT_H
#define DISABLEPATCHEVENT_H

#include <string>
#include "QueuedEvent.hpp"

using std::string;

namespace Ingen {

class Patch;


/** Disables a Patch's DSP processing.
 *
 * \ingroup engine
 */
class DisablePatchEvent : public QueuedEvent
{
public:
	DisablePatchEvent(Engine& engine, SharedPtr<Shared::Responder> responder, SampleCount timestamp, const string& patch_path);
	
	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	string _patch_path;
	Patch* _patch;
};


} // namespace Ingen


#endif // DISABLEPATCHEVENT_H
