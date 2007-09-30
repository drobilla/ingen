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

#ifndef SETPOLYPHONICEVENT_H
#define SETPOLYPHONICEVENT_H

#include <string>
#include <raul/Array.hpp>
#include "QueuedEvent.hpp"

using std::string;

namespace Ingen {

class GraphObject;


/** Delete all nodes from a patch.
 *
 * \ingroup engine
 */
class SetPolyphonicEvent : public QueuedEvent
{
public:
	SetPolyphonicEvent(Engine& engine, SharedPtr<Responder> responder, FrameTime time, QueuedEventSource* source, const string& path, bool poly);
	
	void pre_process();
	void execute(ProcessContext& context);
	void post_process();

private:
	string       _path;
	GraphObject* _object;
	bool         _poly;
};


} // namespace Ingen


#endif // SETPOLYPHONICEVENT_H
