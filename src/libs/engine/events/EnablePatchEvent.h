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

#ifndef ENABLEPATCHEVENT_H
#define ENABLEPATCHEVENT_H

#include <string>
#include "QueuedEvent.h"

using std::string;

namespace Raul { template <typename T> class Array; }

namespace Ingen {

class Patch;
class Node;


/** Enables a patch's DSP processing.
 *
 * \ingroup engine
 */
class EnablePatchEvent : public QueuedEvent
{
public:
	EnablePatchEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& patch_path);
	
	void pre_process();
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	string        _patch_path;
	Patch*        _patch;
	Raul::Array<Node*>* _process_order; // Patch's new process order
};


} // namespace Ingen


#endif // ENABLEPATCHEVENT_H
