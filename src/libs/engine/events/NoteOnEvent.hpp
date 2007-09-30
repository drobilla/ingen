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

#ifndef NOTEONEVENT_H
#define NOTEONEVENT_H

#include "Event.hpp"
#include "types.hpp"
#include <string>
using std::string;

namespace Ingen {

class Node;


/** A note on event.
 *
 * \ingroup engine
 */
class NoteOnEvent : public Event
{
public:
	NoteOnEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, Node* patch, uchar note_num, uchar velocity);
	NoteOnEvent(Engine& engine, SharedPtr<Responder> responder, SampleCount timestamp, const string& node_path, uchar note_num, uchar velocity);
	
	void execute(ProcessContext& context);
	void post_process();

private:
	Node*  _node;
	string _node_path;
	uchar  _note_num;
	uchar  _velocity;
	bool   _is_osc_triggered;
};


} // namespace Ingen

#endif // NOTEONEVENT_H
