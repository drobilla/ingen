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

#ifndef NOTEOFFEVENT_H
#define NOTEOFFEVENT_H

#include "Event.h"
#include "types.h"
#include <string>
using std::string;

namespace Ingen {

class Node;


/** A note off event.
 *
 * \ingroup engine
 */
class NoteOffEvent : public Event
{
public:
	NoteOffEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, Node* node, uchar note_num);
	NoteOffEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, uchar note_num);
	
	void execute(SampleCount nframes, FrameTime start, FrameTime end);
	void post_process();

private:
	Node*  _node;
	string _node_path;
	uchar  _note_num;
};


} // namespace Ingen

#endif // NOTEOFFEVENT_H
